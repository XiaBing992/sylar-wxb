/*
 * @Author: XiaBing
 * @Date: 2024-01-12 10:23:09
 * @LastEditors: Xiabing
 * @LastEditTime: 2024-02-01 19:37:57
 * @FilePath: /sylar-wxb/sylar/fiber.cc
 * @Description: 
 */
#include <atomic>
#include <bits/stdint-uintn.h>
#include <cstddef>
#include <exception>
#include <functional>
#include <ucontext.h>

#include "fiber.h"
#include "config.h"
#include "log.h"
#include "util.h"
#include "scheduler.h"

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id(0);
static std::atomic<uint64_t> s_fiber_count(0);

static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
  Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

class MallocStackAllocator
{
public:
  static void* Alloc(size_t size)
  {
    return malloc(size);
  }

  static void Dealloc(void* vp, size_t size)
  {
    return free(vp);
  }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId()
{
  if (t_fiber)
  {
    return t_fiber->getId();
  }
  return 0;
}

Fiber::Fiber()
{
  state_ = EXEC;
  SetThis(this);
  if (getcontext(&ctx_))
  {
    SYLAR_ASSERT2(false, "getcontext");
  }
  s_fiber_count++;

  SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
  : id_(++s_fiber_id), cb_(cb)
{
  s_fiber_count++;
  stacksize_ = stacksize ? stacksize : g_fiber_stack_size->getValue();

  stack_ = StackAllocator::Alloc(stacksize_);
  if (getcontext(&ctx_))
  {
    SYLAR_ASSERT2(false, "getcontext");
  }
  ctx_.uc_link = nullptr; // 下一个要执行的上下文
  ctx_.uc_stack.ss_sp = stack_; // 堆栈起始指针
  ctx_.uc_stack.ss_size = stacksize_;

  if (!use_caller)
  {
    makecontext(&ctx_, &Fiber::MainFunc, 0);
  }
  else
  {
    makecontext(&ctx_, &Fiber::CallerMainFunc, 0);
  }

  SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << id_;
}

Fiber::~Fiber()
{
  s_fiber_count--;
  if (stack_)
  {
    SYLAR_ASSERT(state_ == TERM
                || state_ == EXCEPT
                || state_ == INIT);
    StackAllocator::Dealloc(stack_, stacksize_);
  }
  else
  {
    SYLAR_ASSERT(!cb_);
    SYLAR_ASSERT(state_ == EXEC);
    Fiber* cur = t_fiber;
    if (cur == this)
    {
      SetThis(nullptr);
    }
  }
  SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id = " << id_ << " total = " << s_fiber_count;
}

void Fiber::reset(std::function<void()> cb)
{
  SYLAR_ASSERT(stack_);
  SYLAR_ASSERT(state_ == TERM
          || state_ == EXCEPT
          || state_ == INIT);
  cb_ = cb;
  ctx_.uc_link = nullptr;
  ctx_.uc_stack.ss_sp = stack_;
  ctx_.uc_stack.ss_size = stacksize_;

  makecontext(&ctx_, &Fiber::MainFunc, 0);
  state_ = INIT;
}

void Fiber::call()
{
  SYLAR_LOG_INFO(g_logger) << "call";
  SetThis(this);
  if (swapcontext(&t_threadFiber->ctx_, &ctx_))
  {
    SYLAR_ASSERT2(false, "swapcontext");
  }
}

void Fiber::back()
{
  SetThis(t_threadFiber.get());
  if (swapcontext(&ctx_, &t_threadFiber->ctx_))
  {
    SYLAR_ASSERT2(false, "swapcontext");
  }
}

void Fiber::swapIn()
{
  SetThis(this);
  SYLAR_ASSERT(state_ != EXEC);
  state_ = EXEC;
  if (swapcontext(&Scheduler::GetMainFiber()->ctx_, &ctx_)) // 让当前线程执行的协程暂停，转而执行this的栈空间
  {
    SYLAR_ASSERT2(false, "swapcontext");
  }
}

void Fiber::swapOut()
{
  SetThis(Scheduler::GetMainFiber());
  if(swapcontext(&ctx_, &Scheduler::GetMainFiber()->ctx_)) // 将当前上下文变为空
  {
    SYLAR_ASSERT2(false, "swapcontext");
  }
}

void Fiber::SetThis(Fiber* f)
{
  t_fiber = f;
}

Fiber::ptr Fiber::GetThis()
{
  if (t_fiber) return t_fiber->shared_from_this();

  Fiber::ptr main_fiber(new Fiber);
  SYLAR_ASSERT(t_fiber == main_fiber.get()); // 这里？？？
  t_threadFiber = main_fiber;
  return t_fiber->shared_from_this();
}

void Fiber::YieldToReady()
{
  Fiber::ptr cur = GetThis();
  SYLAR_ASSERT(cur->state_ == EXEC);
  cur->state_ = READY;
  cur->swapOut(); 
}

void Fiber::YieldToHold() 
{
  Fiber::ptr cur = GetThis();
  SYLAR_ASSERT(cur->state_ == EXEC);
  cur->swapOut();
}

uint64_t Fiber::TotalFibers()
{
  return s_fiber_count;
}

void Fiber::MainFunc()
{
  Fiber::ptr cur = GetThis();
  SYLAR_ASSERT(cur);
  try {
    cur->cb_();
    cur->cb_ = nullptr;
    cur->state_ = TERM;
  } catch(std::exception& ex) {
    cur->state_ = EXCEPT;
    SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() << " fiber_id=" << cur->getId()
      << std::endl << sylar::BacktraceToString();
  } catch (...) {
    cur->state_ = EXCEPT;
    SYLAR_LOG_ERROR(g_logger) << "Fiber Except" << " fiber_id=" << cur->getId()
      << std::endl << sylar::BacktraceToString();
  }
  auto raw_ptr = cur.get();
  cur.reset();
  raw_ptr->swapOut();

  SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}
void Fiber::CallerMainFunc()
{
  Fiber::ptr cur = GetThis();
  SYLAR_ASSERT(cur);
  try {
    cur->cb_();
    cur->cb_ = nullptr;
    cur->state_ = TERM;
  } catch (std::exception& ex) {
    cur->state_ = EXCEPT;
    SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() << " fiber_id=" << cur->getId()
      << std::endl << sylar::BacktraceToString();
  } catch(...) {
    cur->state_ = EXCEPT;
    SYLAR_LOG_ERROR(g_logger) << "Fiber Except" << " fiber_id=" << cur->getId()
      << std::endl << sylar::BacktraceToString();
  }

  auto raw_ptr = cur.get();
  cur.reset();
  raw_ptr->back();
  SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

}