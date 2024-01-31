/*
 * @Author: XiaBing
 * @Date: 2024-01-29 20:16:38
 * @LastEditors: Xiabing
 * @LastEditTime: 2024-01-31 22:12:28
 * @FilePath: /sylar-wxb/sylar/scheduler.h
 * @Description: 
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <cstddef>
#include <functional>
#include <ostream>
#include <vector>

#include "log.h"
#include "mutex.h"
#include "thread.h"
#include "fiber.h"

namespace sylar {

class Scheduler
{
public:
  typedef std::shared_ptr<Scheduler> ptr;
  typedef Mutex MutexType;

  /**
   * @brief 
   * @param threads 线程数
   * @param use_caller 是否使用当前线程调用线程
   * @param name 协程调度器名称
   */  
  Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");

  virtual ~Scheduler();

  const std::string& getName() const { return name_;}
  
  /**
   * @brief 返回当前协程调度器 
   */  
  static Scheduler* GetThis();

  /**
   * @brief 返回当前协程调度器的调度协程 
   */  
  static Fiber* GetMainFiber();

  /**
   * @brief 让线程池运行调度器的run函数 
   */  
  void start();

  void stop();

  /**
   * @brief 调度协程
   * @param fc 协程或函数
   * @param thread 协程执行的线程id，-1标识任意线程  
   */  
  template<typename FiberOrCb>
  void schedule(FiberOrCb fc, int thread = - 1)
  {
    bool need_tickle = false;
    {
      MutexType::Lock lock(mutex_);
      need_tickle = scheduleNoLock(fc, thread);
    }

    if (need_tickle) tickle();
  }

  /**
   * @brief 批量调度协程
   * @param 协程数组的开始
   * @param 协程数组的结束 
   */  
  template<typename InputIterator>
  void schedule(InputIterator begin, InputIterator end)
  {
    bool need_tickle = false;
    {
      MutexType::Lock lock(mutex_);
      while(begin != end)
      {
        need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
        begin++;
      }
    }

    if (need_tickle) tickle();
  }

  void switchTo(int thread = -1);
  std::ostream& dump(std::ostream& os);

protected:

  /**
   * @brief 通知协程调度器有任务 
   */  
  virtual void tickle();

  void run();

  /**
   * @brief 返回是否可以停止 
   */  
  virtual bool stopping();

  /**
   * @brief 协程无任务可调度时执行idle协程 
   */  
  virtual void idle();

  /**
   * @brief 设置当前协程调度器
   */  
  void setThis();

  /**
   * @brief 是否有空闲线程 
   */  
  bool hasIdleThreads() { return idleThreadCount_ > 0;}


private:
/**
 * @brief 协程调度启动（无锁） 
 */  
template<typename FiberOrCb>
bool scheduleNoLock(FiberOrCb fc, int thread)
{
  bool need_tickle = fibers_.empty(); // 之前没任务，需要重新唤醒
  FiberAndThread ft(fc, thread);

  if (ft.fiber_ || ft.cb_) fibers_.push_back(ft); // 将要执行的任务放进去

  return need_tickle;
}

private:
  struct FiberAndThread
  {
    Fiber::ptr fiber_; // 协程
    
    std::function<void()> cb_; // 回调函数

    int thread_; //当前协程在哪个线程上执行

    FiberAndThread(Fiber::ptr fiber, int thread)
      : fiber_(fiber), thread_(thread) {}

    FiberAndThread(Fiber::ptr* f, int thread) : thread_(thread)
    {
      fiber_.swap(*f);
    }

    FiberAndThread(std::function<void()> f, int thread)
      : cb_(f), thread_(thread) {}

    FiberAndThread(std::function<void()>* f, int thread) : thread_(thread)
    {
      cb_.swap(*f);
    }

    FiberAndThread() : thread_(-1) {}

    void reset()
    {
      fiber_ = nullptr;
      cb_ = nullptr;
      thread_ = -1;
    }
  };



private:
  MutexType mutex_;

  std::vector<Thread::ptr> threads_; // 线程池

  std::list<FiberAndThread> fibers_; // 待执行的协程队列

  Fiber::ptr rootFiber_; // 调度协程，use_caller为true时有效

  std::string name_; // 协程调度器名称

protected:
  std::vector<int> threadIds_; // 协程下的线程id数组

  size_t threadCount_ = 0; // 线程数量

  std::atomic<size_t> activeThreadCount_ = {0}; // 工作线程数量

  std::atomic<size_t> idleThreadCount_ = {0}; // 空闲线程数量

  bool stopping_ = true; // 是否正在停止

  bool autoStop_ = false; // 是否主动停止

  int rootThread_ = 0; // 主线程id，即use_caller的id
};

class SchedulerSwitcher : public Noncopyable
{
public:
  SchedulerSwitcher(Scheduler* target = nullptr);
  ~SchedulerSwitcher();

private:
  Scheduler* caller_;
};

} // namespace sylar

#endif