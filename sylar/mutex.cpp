/*
 * @Author: XiaBing
 * @Date: 2024-01-03 22:21:54
 * @LastEditors: WXB 1763567512@qq.com
 * @LastEditTime: 2024-02-06 20:40:34
 * @FilePath: /sylar-wxb/sylar/mutex.cpp
 * @Description: 
 */
#include "mutex.h"
#include <bits/stdint-uintn.h>
#include <stdexcept>
namespace sylar {

Semaphore::Semaphore(uint32_t count)
{
  if (sem_init(&semaphore_, 0, count))
  {
    throw std::logic_error("sem_init error");
  }
}

Semaphore::~Semaphore()
{
  sem_destroy(&semaphore_);
}

void Semaphore::wait()
{
  if (sem_wait(&semaphore_))
  {
    throw std::logic_error("sem_wait error");
  }
}

void Semaphore::notify()
{
  if (sem_post(&semaphore_))
  {
    throw std::logic_error("sem_post error");
  }
}
} // namespace sylar