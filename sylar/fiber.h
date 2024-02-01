/*
 * @Author: XiaBing
 * @Date: 2024-01-12 10:21:51
 * @LastEditors: Xiabing
 * @LastEditTime: 2024-01-31 21:35:31
 * @FilePath: /sylar-wxb/sylar/fiber.h
 * @Description: 
 */
#ifndef FIBER_H
#define FIBER_H

#include <ucontext.h>
#include <functional>
#include <memory>

#include "macro.h"

namespace sylar {

class Scheduler;

class Fiber : public std::enable_shared_from_this<Fiber>
{
friend class Scheduler;
public:
  typedef std::shared_ptr<Fiber> ptr;

  /**
  * @brief 协程状态 
  * @details INIT 初始化
  *          HOLD 暂停
  *          EXEC 执行中 
  *          TERM 结束
  *          READY 可执行
  *          EXCEPT 异常状态
  */ 
  enum State
  {
    INIT,
    HOLD, 
    EXEC,
    TERM,
    READY,
    EXCEPT,
  };

private:

  Fiber();

public:
  /**
   * @brief 构造函数
   * @param cb 协程执行的函数
   * @param stacksize 协程栈大小
   * @param use_caller 是否在MainFiber上调度
   */  
  Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);

  ~Fiber();

  /**
   * @brief 重置协程执行函数，并设置状态 
   * @pre getState() 为INIT, TERM, EXCEPT
   * @post getState() = INIT
   */  
  void reset(std::function<void()> cb);

  /**
   * @brief 将当前协程切换到运行状态
   * @pre getState() != EXEC
   * @post getState() = EXEC
   */  
  void swapIn();

  /**
   * @brief 将当前协程切换到后台 
   */  
  void swapOut();

  /**
   * @brief 将当前线程切换到执行状态 
   * @pre 执行的为当前线程的主协程
   */  
  void call();

  /**
   * @brief 将当前线程切换到后台
   * @pre 执行的为该协程
   * @post 返回到线程的主协程 
   */  
  void back();

  uint64_t getId() const { return id_;}

  State getState() const { return state_;}

public:
  /**
   * @brief 设置当前线程的运行协程 
   * @param f 运行协程
   */  
  static void SetThis(Fiber* f);

  /**
   * @brief 返回当前所在的线程的协程
   */  
  static Fiber::ptr GetThis();

  /**
   * @brief 将当前协程切换到后台，并设置为READY状态
   */  
  static void YieldToReady();

  /**
   * @brief 将当前协程切换到后台，并设置为HOLD状态 
   */  
  static void YieldToHold();

  /**
   * @brief 返回当前协程数量 
   */  
  static uint64_t TotalFibers();

  /**
   * @brief 协程执行函数
   * @post 执行完成返回到线程主协程 
   */  
  static void MainFunc();

  /**
   * @brief 协程执行函数
   * @post 执行完成返回到线程调度协程 
   */  
  static void CallerMainFunc();

  /**
   * @brief 获取当前协程的id 
   */  
  static uint64_t GetFiberId();

private:
  uint64_t id_ = 0; // 协程id

  uint32_t stacksize_; // 协程运行栈大小

  State state_ = INIT;// 协程状态

  ucontext_t ctx_; // 协程上下文

  void* stack_ = nullptr; // 协程运行栈指针

  std::function<void()> cb_; // 协程运行函数
};

}


#endif
