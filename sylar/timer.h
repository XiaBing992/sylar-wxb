/*
 * @Author: Xiabing
 * @Date: 2024-02-07 09:30:05
 * @LastEditors: WXB 1763567512@qq.com
 * @LastEditTime: 2024-02-07 09:42:16
 * @FilePath: /sylar-wxb/sylar/timer.h
 * @Description: 
 * 
 * Copyright (c) 2024 by Xiabing, All Rights Reserved. 
 */
#ifndef TIMER_H
#define TIMER_H

#include <cstdint>
#include <memory>
#include <functional>
#include <vector>
#include <set>

#include "mutex.h"

namespace sylar {

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer>
{
friend class TimerManager;
public:
  typedef std::shared_ptr<Timer> ptr;

  /**
   * @brief 取消定时器 
   */  
  bool cancel();

  /**
   * @brief 刷新定时器时间 
   */  
  bool refresh();

  /**
   * @brief 重置定时器时间 
   * @param ms 定时器执行间隔
   * @param from_now 是否从当前时间开始计算
   */  
  bool reset(uint64_t ms, bool from_now);

private:
  /**
  * @brief 构造函数
  * @param ms 定时器执行间隔时间
  * @param cb 回调函数
  * @param recurring 是否循环
  * @param manager 定时器管理器
  */
  Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager);

  /**
  * @brief 构造函数
  * @param next 执行的时间戳(毫秒)
  */
  Timer(uint64_t next);

private:
  /// 是否循环定时器
  bool recurring_ = false;
  /// 执行周期
  uint64_t ms_ = 0;
  /// 精确的执行时间
  uint64_t next_ = 0;
  /// 回调函数
  std::function<void()> cb_;
  /// 定时器管理器
  TimerManager* manager_ = nullptr;

private:
  /**
  * @brief 定时器比较仿函数
  */
  struct Comparator
  {
    /**
    * @brief 比较定时器的智能指针的大小(按执行时间排序)
    * @param[in] lhs 定时器智能指针
    * @param[in] rhs 定时器智能指针
    */
    bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
  };
};

class TimerManager
{
friend class Timer;
public:
  /// 读写锁类型
  typedef RWMutex RWMutexType;

  /**
  * @brief 构造函数
  */
  TimerManager();

  /**
  * @brief 析构函数
  */
  virtual ~TimerManager();

  /**
  * @brief 添加定时器
  * @param[in] ms 定时器执行间隔时间
  * @param[in] cb 定时器回调函数
  * @param[in] recurring 是否循环定时器
  */
  Timer::ptr addTimer(uint64_t ms, std::function<void()> cb
                      ,bool recurring = false);

  /**
  * @brief 添加条件定时器
  * @param[in] ms 定时器执行间隔时间
  * @param[in] cb 定时器回调函数
  * @param[in] weak_cond 条件
  * @param[in] recurring 是否循环
  */
  Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb
                      ,std::weak_ptr<void> weak_cond
                      ,bool recurring = false);

  /**
   * @brief 到最近一个定时器执行的时间间隔(毫秒)
   */
  uint64_t getNextTimer();
  

  /**
   * @brief 获取需要执行的定时器的回调函数列表
   * @param[out] cbs 回调函数数组
   */
  void listExpiredCb(std::vector<std::function<void()> >& cbs);

  /**
   * @brief 是否有定时器
   */
  bool hasTimer();
protected:

  /**
   * @brief 当有新的定时器插入到定时器的首部,执行该函数
   */
  virtual void onTimerInsertedAtFront() = 0;

  /**
   * @brief 将定时器添加到管理器中
   */
  void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);
private:
  /**
   * @brief 检测服务器时间是否溢出
   */
  bool detectClockRollover(uint64_t now_ms);
private:
  /// Mutex
  RWMutexType mutex_;
  /// 定时器集合
  std::set<Timer::ptr, Timer::Comparator> timers_;
  /// 是否触发onTimerInsertedAtFront
  bool tickled_ = false;
  /// 上次执行时间
  uint64_t previouseTime_ = 0;

};

} // namespace sylar


#endif