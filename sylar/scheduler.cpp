#include <cstddef>
#include <functional>
#include <ostream>
#include <vector>
#include <string>

#include "log.h"
#include "mutex.h"
#include "scheduler.h"
#include "macro.h"
#include "thread.h"
#include "util.h"
#include "hook.h"


namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_scheduler_fiber = nullptr; //调度协程的上下文

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
  : name_(name)
{
  SYLAR_ASSERT(threads > 0);

  if (use_caller)
  {
    sylar::Fiber::GetThis();
    threads--;

    SYLAR_ASSERT(GetThis() == nullptr);
    t_scheduler = this;

    rootFiber_.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
    sylar::Thread::SetName(name_);

    t_scheduler_fiber = rootFiber_.get();
    rootThread_ = sylar::GetThreadId();
    threadIds_.push_back(rootThread_);
  }
  else
  {
    rootThread_ = -1;
  }

  threadCount_ = threads;
}

Scheduler::~Scheduler()
{
  SYLAR_ASSERT(stopping_);

  if (GetThis() == this)
  {
    t_scheduler = nullptr;
  }
}

Scheduler* Scheduler::GetThis()
{
  return t_scheduler;
}

Fiber* Scheduler::GetMainFiber()
{
  return t_scheduler_fiber;
}


void Scheduler::start()
{
  MutexType::Lock lock(mutex_);

  if (!stopping_) return;

  stopping_ = false;
  SYLAR_ASSERT(threads_.empty());

  threads_.resize(threadCount_);

  for (size_t i = 0; i < threadCount_; i++)
  {
    threads_[i].reset(new Thread(std::bind(&Scheduler::run, this), name_ + "_" + std::to_string(i)));
    threadIds_.push_back(threads_[i]->getId());
  }
  lock.unlock();

}

void Scheduler::stop()
{
  autoStop_ = true;
  if (rootFiber_ && threadCount_ == 0
      && (rootFiber_->getState() == Fiber::TERM || rootFiber_->getState() == Fiber::INIT))
  {
    SYLAR_LOG_INFO(g_logger) << this << "stopped";
    stopping_ = true;

    if (stopping()) return;
  }

  if (rootThread_ != -1)
  {
    SYLAR_ASSERT(GetThis() == this);
  }
  else
  {
    SYLAR_ASSERT(GetThis() != this);
  }

  stopping_ = true;
  for (size_t i = 0; i < threadCount_; i++)
  {
    tickle();
  }

  if (rootFiber_) tickle();

  if (rootFiber_)
  {
    if (!stopping()) rootFiber_->call();
  }

  std::vector<Thread::ptr> thrs;
  {
    MutexType::Lock lock(mutex_);
    thrs.swap(threads_);
  }

  for (auto& i : thrs) i->join();
}

void Scheduler::setThis()
{
  t_scheduler = this; // 设置当前线程的调度器地址（所有线程的调度器应该是同一个）
}

void Scheduler::run()
{
  SYLAR_LOG_DEBUG(g_logger) << name_ << " run";
  set_hook_enable(true);
  setThis();
  if (sylar::GetThreadId() != rootThread_) // 当前线程不是主线程
  {
    t_scheduler_fiber = Fiber::GetThis().get(); // 拿到当前线程的执行协程
  }

  Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this))); // 用于其他没有执行的空闲任务
  Fiber::ptr cb_fiber; //用于将回调函数情况

  FiberAndThread ft;
  while (true)
  {
    ft.reset();
    bool tickle_me = false;
    bool is_active = false;
    {
      MutexType::Lock lock(mutex_);
      auto it = fibers_.begin();
      while (it != fibers_.end())
      {
        if (it->thread_ != -1 && it->thread_ != sylar::GetThreadId()) // 当前协程不是在本线程上执行
        {
          it++;
          tickle_me = true;
          continue;
        }

        SYLAR_ASSERT(it->fiber_ || it->cb_);
        if (it->fiber_ && it->fiber_->getState() == Fiber::EXEC)
        {
          it++;
          continue;
        }

        ft = *it;
        fibers_.erase(it++);
        activeThreadCount_++;
        is_active = true;
        break;
      }
      tickle_me |= it != fibers_.end();
    }

    if (tickle_me) tickle();

    if (ft.fiber_ && (ft.fiber_->getState() != Fiber::TERM && ft.fiber_->getState() != Fiber::EXCEPT))
    {
      ft.fiber_->swapIn();
      activeThreadCount_--;

      if (ft.fiber_->getState() == Fiber::READY)
      {
        schedule(ft.fiber_);
      }
      else if (ft.fiber_->getState() != Fiber::TERM && ft.fiber_->getState() != Fiber::EXCEPT)
      {
        ft.fiber_->state_ = Fiber::HOLD;
      }
      ft.reset();
    }
    else if (ft.cb_)
    {
      if (cb_fiber) cb_fiber->reset(ft.cb_);
      else cb_fiber.reset(new Fiber(ft.cb_));
      ft.reset();
      cb_fiber->swapIn();
      activeThreadCount_--;
      if (cb_fiber->getState() == Fiber::READY)
      {
        schedule(cb_fiber);
        cb_fiber.reset();
      }
      else if (cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM)
      {
        cb_fiber->reset(nullptr);
      }
      else
      {
        cb_fiber->state_ = Fiber::HOLD;
        cb_fiber.reset();
      }
    }
    else // 无任务执行
    {
      if (is_active)
      {
        activeThreadCount_--;
        continue;
      }
      if (idle_fiber->getState() == Fiber::TERM)
      {
        SYLAR_LOG_INFO(g_logger) << "idle fiber term";
        break;
      }

      idleThreadCount_++;
      idle_fiber->swapIn(); // 运行idle协程
      idleThreadCount_--;
      if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT)
      {
        idle_fiber->state_ = Fiber::HOLD;
      }
    }
  }
}

void Scheduler::tickle()
{
  SYLAR_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping()
{
  MutexType::Lock lock(mutex_);
  return autoStop_ && stopping_ && fibers_.empty() && activeThreadCount_ == 0;
}
void Scheduler::idle()
{
  SYLAR_LOG_INFO(g_logger) << "idle";
  while (!stopping())
  {
    sylar::Fiber::YieldToHold();
  }
  SYLAR_LOG_INFO(g_logger) << "idle end";
}

void Scheduler::switchTo(int thread)
{
  SYLAR_ASSERT(Scheduler::GetThis() != nullptr);
  if (Scheduler::GetThis() == this)
  {
    if (thread == -1 || thread == sylar::GetThreadId())
    {
      return;
    }
  }
  schedule(Fiber::GetThis(), thread);
  Fiber::YieldToHold();
}

std::ostream& Scheduler::dump(std::ostream &os)
{
  os << "[Scheduler name=" << name_ << " size=" << threadCount_ << " active_count=" << activeThreadCount_
    << " idle_count=" << idleThreadCount_ << " stopping=" << stopping_
    << " ]" << std::endl << "    ";
  for(size_t i = 0; i < threadIds_.size(); ++i) 
  {
    if(i) 
    {
        os << ", ";
    }
    os << threadIds_[i];
  }
  return os;
}

SchedulerSwitcher::SchedulerSwitcher(Scheduler* target)
{
  caller_ = Scheduler::GetThis();
  if (target)
  {
    target->switchTo();
  }
}

SchedulerSwitcher::~SchedulerSwitcher()
{
  if (caller_)
  {
    caller_->switchTo();
  }
}

} // namespce sylar