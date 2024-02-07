/*
 * @Author: Xiabing
 * @Date: 2024-02-07 09:37:45
 * @LastEditors: WXB 1763567512@qq.com
 * @LastEditTime: 2024-02-07 21:36:28
 * @FilePath: /sylar-wxb/sylar/timer.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by Xiabing, All Rights Reserved. 
 */
#include "timer.h"
#include "util.h"

namespace sylar {
bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const
{
  if(!lhs && !rhs)
  {
    return false;
  }
  if(!lhs)
  {
    return true;
  }
  if(!rhs)
  {
    return false;
  }
  if(lhs->next_ < rhs->next_)
  {
    return true;
  }
  if(rhs->next_ < lhs->next_)
  {
    return false;
  }
  return lhs.get() < rhs.get();
}

Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager)
    :recurring_(recurring), ms_(ms), cb_(cb), manager_(manager)
{
  next_ = sylar::GetCurrentMS() + ms_;
}

Timer::Timer(uint64_t next) : next_(next)
{

}

bool Timer::cancel()
{
  TimerManager::RWMutexType::WriteLock lock(manager_->mutex_);
  if(cb_)
  {
    cb_ = nullptr;
    auto it = manager_->timers_.find(shared_from_this());
    manager_->timers_.erase(it);
    return true;
  }
  return false;
}

bool Timer::refresh()
{
  TimerManager::RWMutexType::WriteLock lock(manager_->mutex_);
  if(!cb_)
  {
    return false;
  }
  auto it = manager_->timers_.find(shared_from_this());
  if(it == manager_->timers_.end()) {
      return false;
  }
  manager_->timers_.erase(it);
  next_ = sylar::GetCurrentMS() + ms_;
  manager_->timers_.insert(shared_from_this()); // 为什么前面要删了？？？
  return true;
}

bool Timer::reset(uint64_t ms, bool from_now)
{
  if(ms_ == ms && !from_now)
  {
    return true;
  }
  TimerManager::RWMutexType::WriteLock lock(manager_->mutex_);
  if(!cb_)
  {
    return false;
  }
  auto it = manager_->timers_.find(shared_from_this());
  if(it == manager_->timers_.end())
  {
    return false;
  }
  manager_->timers_.erase(it);

  uint64_t start = 0;
  if(from_now) start = sylar::GetCurrentMS();
  else start = next_ - ms;
  ms_ = ms;
  next_ = start + ms;
  manager_->addTimer(shared_from_this(), lock);

  return true;
}

TimerManager::TimerManager()
{
  previouseTime_ = sylar::GetCurrentMS();
}

TimerManager::~TimerManager()
{

}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
{
  Timer::ptr timer(new Timer(ms, cb, recurring, this));
  RWMutexType::WriteLock lock(mutex_);
  addTimer(timer, lock);
  return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
{
  std::shared_ptr<void> tmp = weak_cond.lock();
  if(tmp)
  {
    cb();
  }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb,
                                            std::weak_ptr<void> weak_cond, bool recurring)
{
  return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

uint64_t TimerManager::getNextTimer()
{
  RWMutexType::ReadLock lock(mutex_);
  tickled_ = false;
  if(timers_.empty())
  {
    return ~0ull;
  }

  const Timer::ptr& next = *timers_.begin();
  uint64_t now_ms = sylar::GetCurrentMS();

  if(now_ms >= next->next_) return 0;
  else return next->next_ - now_ms;
}

void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs)
{
  uint64_t now_ms = sylar::GetCurrentMS();
  std::vector<Timer::ptr> expired;
  {
    RWMutexType::ReadLock lock(mutex_);
    if(timers_.empty())
    {
      return;
    }
  }
  RWMutexType::WriteLock lock(mutex_);
  if(timers_.empty())
  {
    return;
  }
  bool rollover = detectClockRollover(now_ms);
  if(!rollover && ((*timers_.begin())->next_ > now_ms)) return; // 还没到第一个定时器回调函数的执行时间

  Timer::ptr now_timer(new Timer(now_ms));
  auto it = rollover ? timers_.end() : timers_.lower_bound(now_timer);
  while(it != timers_.end() && (*it)->next_ == now_ms) // 找到所有已失效的定时器回调函数
  {
    ++it;
  }
  expired.insert(expired.begin(), timers_.begin(), it);
  timers_.erase(timers_.begin(), it);
  cbs.reserve(expired.size());

  for(auto& timer : expired)
  {
    cbs.push_back(timer->cb_);
    if(timer->recurring_) // 周期函数重新放入
    {
      timer->next_ = now_ms + timer->ms_;
      timers_.insert(timer);
    }
    else
    {
      timer->cb_ = nullptr;
    }
  }
}

void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock)
{
  auto it = timers_.insert(val).first;
  bool at_front = (it == timers_.begin()) && !tickled_;
  if(at_front)
  {
    tickled_ = true;
  }
  lock.unlock();

  if(at_front)
  {
    onTimerInsertedAtFront();
  }
}

bool TimerManager::detectClockRollover(uint64_t now_ms)
{
  bool rollover = false;
  if(now_ms < previouseTime_ && now_ms < (previouseTime_ - 60 * 60 * 1000)) // ???为什么有两个条件
  {
    rollover = true;
  }
  previouseTime_ = now_ms;
  return rollover;
}

bool TimerManager::hasTimer()
{
  RWMutexType::ReadLock lock(mutex_);
  return !timers_.empty();
}

} // namespace sylar
