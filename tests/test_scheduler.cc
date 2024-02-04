/*
 * @Author: Xiabing
 * @Date: 2024-01-31 21:38:54
 * @LastEditors: Xiabing
 * @LastEditTime: 2024-02-01 20:40:32
 * @FilePath: /sylar-wxb/tests/test_scheduler.cc
 * @Description: 
 */
#include <atomic>

#include "log.h"
#include "mutex.h"
#include "scheduler.h"
#include "thread.h"
#include "util.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_fiber()
{
  static std::atomic<int> s_count = {5};
  SYLAR_LOG_INFO(g_logger) << "test in fiber s_count = " << s_count;

  sleep(2);
  if (--s_count >= 0)
  {
    SYLAR_LOG_DEBUG(g_logger) << "s_count--";
    sylar::Scheduler::GetThis()->schedule(&test_fiber, sylar::GetThreadId());
  }
  SYLAR_LOG_INFO(g_logger) << "end test fiber";
}

int main()
{
  sylar::Thread::SetName("main");
  SYLAR_LOG_INFO(g_logger) << "main";
  sylar::Scheduler sc(1, false, "test");
  sc.start();
  sleep(2);
  SYLAR_LOG_INFO(g_logger) << "schedule";
  sc.schedule(&test_fiber); // 此函数在任意线程上执行
  sc.stop(); 
  SYLAR_LOG_INFO(g_logger) << "over";
  return 0;
}