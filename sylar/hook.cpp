/*
 * @Author: Xiabing
 * @Date: 2024-02-02 11:32:27
 * @LastEditors: WXB 1763567512@qq.com
 * @LastEditTime: 2024-02-08 15:59:38
 * @FilePath: /sylar-wxb/sylar/hook.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by Xiabing, All Rights Reserved. 
 */
#include <cstdint>
#include <dlfcn.h>

#include "hook.h"
#include "log.h"
#include "config.h"
#include "fd_manager.h"
#include "iomanager.h"
#include "macro.h"


sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
namespace sylar {

static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
  sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

#define HOOK_FUNC(XX) \
  XX(sleep) \
  XX(usleep) \
  XX(nanosleep) \
  XX(socket) \
  XX(connect) \
  XX(accept) \
  XX(read) \
  XX(readv) \
  XX(recv) \
  XX(recvfrom) \
  XX(recvmsg) \
  XX(write) \
  XX(writev) \
  XX(send) \
  XX(sendto) \
  XX(sendmsg) \
  XX(close) \
  XX(fcntl) \
  XX(ioctl) \
  XX(getsockopt) \
  XX(setsockopt)

void hook_init()
{
  static bool is_inited = false;
  if (is_inited) return;

#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name); // 返回下一个共享库中名为name的函数指针
  HOOK_FUNC(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter
{
  _HookIniter()
  {
    hook_init();
    s_connect_timeout = g_tcp_connect_timeout->getValue();

    g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value)
    {
      SYLAR_LOG_INFO(g_logger) << "tcp connect timeout changed from"
        << old_value << " to" << new_value;
      s_connect_timeout = new_value;
    });
  }
  
};

static _HookIniter s_hook_initer;

bool is_hook_enable()
{
  return t_hook_enable;
}

void set_hook_enable(bool flag)
{
  t_hook_enable = flag;
}

} // namespace sylar

struct timer_info
{
  int cancelled = 0;
};

template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name, uint32_t event, int timeout_so, Args&&... args)
{
  if(!sylar::t_hook_enable)
  {
    return fun(fd, std::forward<Args>(args)...);
  }

  sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
  if(!ctx)
  {
    return fun(fd, std::forward<Args>(args)...);
  }

  if(ctx->isClose())
  {
    errno = EBADF;
    return -1;
  }

  if(!ctx->isSocket() || ctx->getUserNonblock())
  {
    return fun(fd, std::forward<Args>(args)...);
  }

  uint64_t to = ctx->getTimeout(timeout_so); // 获取超时时间
  std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
  // 尝试执行原始函数直到成功
  ssize_t n = fun(fd, std::forward<Args>(args)...);
  while(n == -1 && errno == EINTR)
  {
    n = fun(fd, std::forward<Args>(args)...);
  }
  if(n == -1 && errno == EAGAIN) // errno表示io需要等待
  {
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    sylar::Timer::ptr timer;
    std::weak_ptr<timer_info> winfo(tinfo);

    if(to != (uint64_t)-1)
    {
      timer = iom->addConditionTimer(to, [winfo, fd, iom, event]()
      {
        auto t = winfo.lock();
        if(!t || t->cancelled)
        {
          return;
        }
        t->cancelled = ETIMEDOUT;
        iom->cancelEvent(fd, (sylar::IOManager::Event)(event));
      }, winfo);
    }

    int rt = iom->addEvent(fd, (sylar::IOManager::Event)(event));
    if(SYLAR_UNLIKELY(rt))
    {
      SYLAR_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
          << fd << ", " << event << ")";
      if(timer)
      {
        timer->cancel();
      }
      return -1;
    } 
    else
    {
      sylar::Fiber::YieldToHold();
      if(timer)
      {
        timer->cancel();
      }
      if(tinfo->cancelled)
      {
        errno = tinfo->cancelled;
        return -1;
      }
      goto retry;
    }
  }
  
  return n;
}

#ifdef __cplusplus
extern "C" {
#endif

#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX


unsigned int sleep(unsigned int seconds)
{
  if(!sylar::t_hook_enable)
  {
    return sleep_f(seconds);
  }

  sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
  sylar::IOManager* iom = sylar::IOManager::GetThis();
  iom->addTimer(seconds * 1000, std::bind((void(sylar::Scheduler::*)
          (sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule, iom, fiber, -1)); // 定时用fiber换入
  sylar::Fiber::YieldToHold();
  return 0;
}

int usleep(useconds_t usec)
{
  if(!sylar::t_hook_enable)
  {
    return usleep_f(usec);
  }
  sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
  sylar::IOManager* iom = sylar::IOManager::GetThis();
  iom->addTimer(usec / 1000, std::bind((void(sylar::Scheduler::*)
                (sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule, iom, fiber, -1));
  sylar::Fiber::YieldToHold();
  return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem)
{
  if(!sylar::t_hook_enable)
  {
    return nanosleep_f(req, rem);
  }

  int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 /1000;
  sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
  sylar::IOManager* iom = sylar::IOManager::GetThis();
  iom->addTimer(timeout_ms, std::bind((void(sylar::Scheduler::*)
          (sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule
          ,iom, fiber, -1));
  sylar::Fiber::YieldToHold();
  return 0;
}

int socket(int domain, int type, int protocol)
{
  if(!sylar::t_hook_enable)
  {
    return socket_f(domain, type, protocol);
  }
  int fd = socket_f(domain, type, protocol);
  if(fd == -1) return fd;

  sylar::FdMgr::GetInstance()->get(fd, true); //创建文件句柄
  return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms)
{
  if(!sylar::t_hook_enable)
  {
    return connect_f(fd, addr, addrlen);
  }
  sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
  if(!ctx || ctx->isClose())
  {
    errno = EBADF;
    return -1;
  }

  if(!ctx->isSocket())
  {
    return connect_f(fd, addr, addrlen);
  }

  if(ctx->getUserNonblock())
  {
    return connect_f(fd, addr, addrlen);
  }

  int n = connect_f(fd, addr, addrlen);
  if(n == 0)
  {
    return 0;
  }
  else if(n != -1 || errno != EINPROGRESS)
  {
    return n;
  }

  sylar::IOManager* iom = sylar::IOManager::GetThis();
  sylar::Timer::ptr timer;
  std::shared_ptr<timer_info> tinfo(new timer_info);
  std::weak_ptr<timer_info> winfo(tinfo);

  if(timeout_ms != (uint64_t)-1) //超时后取消
  {
    timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]()
    {
      auto t = winfo.lock();
      if(!t || t->cancelled)
      {
        return;
      }
      t->cancelled = ETIMEDOUT;
      iom->cancelEvent(fd, sylar::IOManager::WRITE);
    }, winfo);
  }

  int rt = iom->addEvent(fd, sylar::IOManager::WRITE);
  if(rt == 0)
  {
    sylar::Fiber::YieldToHold();
    if(timer) {
        timer->cancel();
    }
    if(tinfo->cancelled)
    {
      errno = tinfo->cancelled;
      return -1;
    }
  }
  else
  {
    if(timer)
    {
      timer->cancel();
    }
    SYLAR_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
  }

  int error = 0;
  socklen_t len = sizeof(int);
  if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len))
  {
    return -1;
  }
  if(!error) return 0;
  else
  {
    errno = error;
    return -1;
  }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  return connect_with_timeout(sockfd, addr, addrlen, sylar::s_connect_timeout);
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
  int fd = do_io(s, accept_f, "accept", sylar::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
  if(fd >= 0)
  {
    sylar::FdMgr::GetInstance()->get(fd, true);
  }
  return fd;
}

#ifdef __cplusplus
}
#endif