/*
 * @Author: Xiabing
 * @Date: 2024-01-01 21:25:56
 * @LastEditors: Xiabing
 * @LastEditTime: 2024-01-30 21:06:53
 * @FilePath: /sylar-wxb/sylar/thread.cc
 * @Description: 
 */
#include <functional>
#include <iterator>
#include <pthread.h>

#include "thread.h"
#include "log.h"
#include "util.h"


namespace sylar {

static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Thread* Thread::GetThis()
{
  return t_thread;
}

const std::string& Thread::GetName()
{
  return t_thread_name;
}

void Thread::SetName(const std::string& name)
{
  if (name.empty())
  {
    return;
  }
  if (t_thread)
  {
    t_thread->name_ = name;
  }
  t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string& name)
                : cb_(cb), name_(name)
{
  if (name.empty())
  {
    name_ = "UNKNOW";
  }
  int rt = pthread_create(&thread_, nullptr, &Thread::run, this);
  if (rt)
  {
    SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt << " name=" << name;
    throw std::logic_error("pthread_create error");
  }
  semaphore_.wait(); // 等待运行
}

Thread::~Thread()
{
  if (thread_)
  {
    pthread_detach(thread_);
  }
}

void Thread::join()
{
  if (thread_)
  {
    int rt = pthread_join(thread_, nullptr);
    if (rt)
    {
      SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt << " name=" << name_;
      throw std::logic_error("pthread_join error");
    }
    thread_ = 0;
  }
}

void* Thread::run(void* arg)
{
  Thread* thread = (Thread*)arg;
  t_thread = thread;
  t_thread_name = thread->name_;
  thread->id_ = sylar::GetThreadId();
  pthread_setname_np(pthread_self(), thread->name_.substr(0, 15).c_str());

  std::function<void()> cb;
  cb.swap(thread->cb_);
  thread->semaphore_.notify();

  cb();

  return 0;
}

} // namespace sylar