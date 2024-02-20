/*
 * @Author: Xiabing
 * @Date: 2024-02-07 11:22:47
 * @LastEditors: WXB 1763567512@qq.com
 * @LastEditTime: 2024-02-08 13:43:21
 * @FilePath: /sylar-wxb/sylar/iomanager.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by Xiabing, All Rights Reserved. 
 */
#include "iomanager.h"
#include "log.h"
#include "macro.h"

#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

enum EpollCtlOp
{

};

static std::ostream& operator<< (std::ostream& os, const EpollCtlOp& op)
{
  switch((int)op)
  {
#define XX(ctl) \
    case ctl: \
        return os << #ctl;
    XX(EPOLL_CTL_ADD);
    XX(EPOLL_CTL_MOD);
    XX(EPOLL_CTL_DEL);
    default:
      return os << (int)op;
  }
#undef XX
}

static std::ostream& operator<< (std::ostream& os, EPOLL_EVENTS events)
{
  if(!events) return os << "0";

  bool first = true;
#define XX(E) \
  if(events & E) { \
    if(!first) { \
      os << "|"; \
    } \
    os << #E; \
    first = false; \
  }
  XX(EPOLLIN);
  XX(EPOLLPRI);
  XX(EPOLLOUT);
  XX(EPOLLRDNORM);
  XX(EPOLLRDBAND);
  XX(EPOLLWRNORM);
  XX(EPOLLWRBAND);
  XX(EPOLLMSG);
  XX(EPOLLERR);
  XX(EPOLLHUP);
  XX(EPOLLRDHUP);
  XX(EPOLLONESHOT);
  XX(EPOLLET);
#undef XX
  return os;
}

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event)
{
  switch(event)
  {
    case IOManager::READ:
      return read;
    case IOManager::WRITE:
      return write;
    default:
      SYLAR_ASSERT2(false, "getContext");
  }
  throw std::invalid_argument("getContext invalid event");
}

void IOManager::FdContext::resetContext(EventContext& ctx)
{
  ctx.scheduler = nullptr;
  ctx.fiber.reset();
  ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event)
{
  //SYLAR_LOG_INFO(g_logger) << "fd=" << fd
  //    << " triggerEvent event=" << event
  //    << " events=" << events;
  SYLAR_ASSERT(events & event);
  //if(SYLAR_UNLIKELY(!(event & event))) {
  //    return;
  //}
  events = (Event)(events & ~event); // 计算与当前事件未重合的事件，赋值给成员变量
  EventContext& ctx = getContext(event);
  if(ctx.cb) ctx.scheduler->schedule(&ctx.cb);
  else ctx.scheduler->schedule(&ctx.fiber);

  ctx.scheduler = nullptr;
  return;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
  :Scheduler(threads, use_caller, name)
{
    epfd_ = epoll_create(5000);
    SYLAR_ASSERT(epfd_ > 0);

    int rt = pipe(tickleFds_);
    SYLAR_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET; // 读时间 边缘触发
    event.data.fd = tickleFds_[0];

    rt = fcntl(tickleFds_[0], F_SETFL, O_NONBLOCK); // 设置文件描述符状态为非阻塞
    SYLAR_ASSERT(!rt);

    rt = epoll_ctl(epfd_, EPOLL_CTL_ADD, tickleFds_[0], &event);
    SYLAR_ASSERT(!rt);

    contextResize(32);

    start();
}

IOManager::~IOManager()
{
  stop();
  close(epfd_);
  close(tickleFds_[0]);
  close(tickleFds_[1]);

  for(size_t i = 0; i < fdContexts_.size(); ++i)
  {
    if(fdContexts_[i])
    {
      delete fdContexts_[i];
    }
  }
}

void IOManager::contextResize(size_t size)
{
  fdContexts_.resize(size);

  for(size_t i = 0; i < fdContexts_.size(); ++i)
  {
    if(!fdContexts_[i])
    {
      fdContexts_[i] = new FdContext;
      fdContexts_[i]->fd = i;
    }
  }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
{
  FdContext* fd_ctx = nullptr;
  RWMutexType::ReadLock lock(mutex_);
  if((int)fdContexts_.size() > fd)
  {
    fd_ctx = fdContexts_[fd];
    lock.unlock();
  }
  else
  {
    lock.unlock();
    RWMutexType::WriteLock lock2(mutex_);
    contextResize(fd * 1.5);
    fd_ctx = fdContexts_[fd];
  }

  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if(SYLAR_UNLIKELY(fd_ctx->events & event)) // 说明事件重复了
  {
    SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd << " event=" << (EPOLL_EVENTS)event
      << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
    SYLAR_ASSERT(!(fd_ctx->events & event));
  }

  int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD; //不为None说明本来就有这个事件，是要修改
  epoll_event epevent;
  epevent.events = EPOLLET | fd_ctx->events | event;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(epfd_, op, fd, &epevent);
  if(rt)
  {
    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << epfd_ << ", " << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
        << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events=" << (EPOLL_EVENTS)fd_ctx->events;
    return -1;
  }

  ++pendingEventCount_;
  fd_ctx->events = (Event)(fd_ctx->events | event);
  FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
  SYLAR_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

  event_ctx.scheduler = Scheduler::GetThis();
  if(cb) event_ctx.cb.swap(cb);
  else
  {
    event_ctx.fiber = Fiber::GetThis();
    SYLAR_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC ,"state=" << event_ctx.fiber->getState());
  }

  return 0;
}

bool IOManager::delEvent(int fd, Event event)
{
  RWMutexType::ReadLock lock(mutex_);
  if((int)fdContexts_.size() <= fd)
  {
    return false;
  }
  FdContext* fd_ctx = fdContexts_[fd];
  lock.unlock();

  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if(SYLAR_UNLIKELY(!(fd_ctx->events & event)))
  {
    return false;
  }

  Event new_events = (Event)(fd_ctx->events & ~event);
  int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL; // 如果还有事件剩余，只用修改当前事件；否则删除这整个事件
  epoll_event epevent;
  epevent.events = EPOLLET | new_events; // 相当于把event的部分掩码设置成0了
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(epfd_, op, fd, &epevent);
  if(rt)
  {
    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << epfd_ << ", " << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
      << rt << " (" << errno << ") (" << strerror(errno) << ")";
    return false;
  }

  --pendingEventCount_;
  fd_ctx->events = new_events;
  FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
  fd_ctx->resetContext(event_ctx); // 将相应的事件删除
  return true;
}

bool IOManager::cancelEvent(int fd, Event event)
{
  RWMutexType::ReadLock lock(mutex_);
  if((int)fdContexts_.size() <= fd)
  {
    return false;
  }
  FdContext* fd_ctx = fdContexts_[fd];
  lock.unlock();

  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if(SYLAR_UNLIKELY(!(fd_ctx->events & event))) return false;

  Event new_events = (Event)(fd_ctx->events & ~event);
  int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = EPOLLET | new_events;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(epfd_, op, fd, &epevent);
  if(rt) {
      SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << epfd_ << ", " << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
          << rt << " (" << errno << ") (" << strerror(errno) << ")";
      return false;
  }

  fd_ctx->triggerEvent(event); // 最后一次触发当前事件
  --pendingEventCount_;
  return true;
}

bool IOManager::cancelAll(int fd)
{
  RWMutexType::ReadLock lock(mutex_);
  if((int)fdContexts_.size() <= fd) {
      return false;
  }
  FdContext* fd_ctx = fdContexts_[fd];
  lock.unlock();

  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if(!fd_ctx->events)
  {
    return false;
  }

  int op = EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = 0;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(epfd_, op, fd, &epevent);
  if(rt)
  {
    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << epfd_ << ", " << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
        << rt << " (" << errno << ") (" << strerror(errno) << ")";
    return false;
  }

  if(fd_ctx->events & READ)
  {
    fd_ctx->triggerEvent(READ);
    --pendingEventCount_;
  }
  if(fd_ctx->events & WRITE)
  {
    fd_ctx->triggerEvent(WRITE);
    --pendingEventCount_;
  }

  SYLAR_ASSERT(fd_ctx->events == 0);
  return true;
}

IOManager* IOManager::GetThis()
{
  return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle()
{
  if(!hasIdleThreads())
  {
    return;
  }
  int rt = write(tickleFds_[1], "T", 1);
  SYLAR_ASSERT(rt == 1);
}

bool IOManager::stopping(uint64_t& timeout)
{
  timeout = getNextTimer();
  return timeout == ~0ull && pendingEventCount_ == 0 && Scheduler::stopping();
}

bool IOManager::stopping()
{
  uint64_t timeout = 0;
  return stopping(timeout);
}

void IOManager::idle()
{
  SYLAR_LOG_DEBUG(g_logger) << "idle";
  const uint64_t MAX_EVNETS = 256;
  epoll_event* events = new epoll_event[MAX_EVNETS]();
  std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr)
  {
    delete[] ptr;
  });

  while(true)
  {
    uint64_t next_timeout = 0; // 得到epoll_wait最大超时时间，之后需要执行定时器函数
    if(SYLAR_UNLIKELY(stopping(next_timeout)))
    {
      SYLAR_LOG_INFO(g_logger) << "name=" << getName() << " idle stopping exit";
      break;
    }

    int rt = 0;
    do {
      static const int MAX_TIMEOUT = 3000;
      if(next_timeout != ~0ull)
      {
        next_timeout = (int)next_timeout > MAX_TIMEOUT? MAX_TIMEOUT : next_timeout;
      } 
      else
      {
        next_timeout = MAX_TIMEOUT;
      }

      rt = epoll_wait(epfd_, events, MAX_EVNETS, (int)next_timeout);
      if(rt < 0 && errno == EINTR) {}
      else break; // 超时或者拿到事件返回
    } while(true);

    std::vector<std::function<void()> > cbs;
    listExpiredCb(cbs); // 找到应该执行的定时器回调函数
    if(!cbs.empty())
    {
      //SYLAR_LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
      schedule(cbs.begin(), cbs.end());
      cbs.clear();
    }

    //if(SYLAR_UNLIKELY(rt == MAX_EVNETS)) {
    //    SYLAR_LOG_INFO(g_logger) << "epoll wait events=" << rt;
    //}

    for(int i = 0; i < rt; ++i)
    {
      epoll_event& event = events[i];
      if(event.data.fd == tickleFds_[0]) // 用于唤醒的管道描述符
      {
        uint8_t dummy[256];
        while(read(tickleFds_[0], dummy, sizeof(dummy)) > 0);
        continue;
      }

      FdContext* fd_ctx = (FdContext*)event.data.ptr;
      FdContext::MutexType::Lock lock(fd_ctx->mutex);
      if(event.events & (EPOLLERR | EPOLLHUP)) // 错误事件
      {
        event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
      }
      int real_events = NONE;
      if(event.events & EPOLLIN)
      {
        real_events |= READ;
      }
      if(event.events & EPOLLOUT)
      {
        real_events |= WRITE;
      }

      if((fd_ctx->events & real_events) == NONE)
      {
        continue;
      }

      int left_events = (fd_ctx->events & ~real_events); // 得到还没有到达的事件（剩余的事件）
      int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
      event.events = EPOLLET | left_events;

      int rt2 = epoll_ctl(epfd_, op, fd_ctx->fd, &event);
      if(rt2)
      {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << epfd_ << ", " << (EpollCtlOp)op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
          << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
        continue;
      }

      //SYLAR_LOG_INFO(g_logger) << " fd=" << fd_ctx->fd << " events=" << fd_ctx->events
      //                         << " real_events=" << real_events;
      if(real_events & READ)
      {
        fd_ctx->triggerEvent(READ);
        --pendingEventCount_;
      }
      if(real_events & WRITE)
      {
        fd_ctx->triggerEvent(WRITE);
        --pendingEventCount_;
      }
    }

    Fiber::ptr cur = Fiber::GetThis();
    auto raw_ptr = cur.get();
    cur.reset();

    raw_ptr->swapOut();
  }
}

void IOManager::onTimerInsertedAtFront()
{
  tickle();
}



} // namespace sylar