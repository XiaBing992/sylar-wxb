/*
 * @Author: XiaBing
 * @Date: 2024-01-03 09:16:02
 * @LastEditors: XiaBing
 * @LastEditTime: 2024-01-03 10:40:06
 * @FilePath: /sylar-wxb/sylar/mutex.h
 * @Description: 
 */
#ifndef MUTEX_H
#define MUTEX_H

#include <cstddef>
#include <pthread.h>
#include <semaphore.h>
#include <bits/stdint-uintn.h>
#include <atomic>
#include <list>

#include "noncopyable.h"

namespace sylar {

/**
 * @brief 信号量 
 */
class Semaphore : Noncopyable
{
public:
  Semaphore(uint32_t count = 0);

  ~Semaphore();

  /**
   * @brief 获取信号量 
   */  
  void wait();

  /**
   * @brief 释放信号量 
   */  
  void notify();

private:
  sem_t semaphore_;
};

/**
 * @brief 局部锁的模板实现 
 */
template<typename T>
struct ScopedLockImpl
{
public:
  ScopedLockImpl(T& mutex) : mutex_(mutex)
  {
    mutex_.lock();
    locked_ = true;
  }

  ~ScopedLockImpl()
  {
    unlock();
  }

  void lock()
  {
    if (!locked_)
    {
      mutex_.lock();
      locked_ = true;
    }
  }

  void unlock()
  {
    if (locked_)
    {
      mutex_.unlock();
      locked_ = false;
    }
  }
private:
  T& mutex_;
  bool locked_;
};

/**
 * @brief 局部读锁
 */
template<typename T>
struct ReadScopedLockImpl
{
public:
  ReadScopedLockImpl(T& mutex) : mutex_(mutex)
  {
    mutex_.rdlock();
    locked_ = true;
  }
  ~ReadScopedLockImpl()
  {
    unlock();
  }

  void lock()
  {
    if (locked_)
    {
      mutex_.rdlock();
      locked_ = true;
    }
  }

  void unlock()
  {
    if (locked_)
    {
      mutex_.unlock();
      locked_ = false;
    }
  }
private:
  T& mutex_;
  bool locked_;
};

/**
 * @brief 局部写锁 
 */
template<typename T>
struct WriteScopedLockImpl
{
public:
  WriteScopedLockImpl(T& mutex) : mutex_(mutex)
  {
    mutex_.wrlock();
    locked_ = true;
  }

  ~WriteScopedLockImpl()
  {
    unlock();
  }

  void lock()
  {
    if (!locked_)
    {
      mutex_.wrlock();
      locked_ = true;
    }
  }

  void unlock()
  {
    if (locked_)
    {
      mutex_.unlock();
      locked_ = false;
    }
  }

private:
  T& mutex_;
  
  bool locked_;
};

/**
 * @brief 互斥量
 */
class Mutex : Noncopyable
{
public:
  typedef ScopedLockImpl<Mutex> Lock;

  Mutex()
  {
    pthread_mutex_init(&mutex_, nullptr);
  }

  ~Mutex()
  {
    pthread_mutex_destroy(&mutex_);
  }

  void lock()
  {
    pthread_mutex_lock(&mutex_);
  }

  void unlock()
  {
    pthread_mutex_unlock(&mutex_);
  }

private:
  pthread_mutex_t mutex_;
};

/**
 * @brief 空锁 用于调试
 */
class NullMutex : Noncopyable
{
public:
  typedef ScopedLockImpl<NullMutex> Lock;

  NullMutex() {}

  ~NullMutex() {}

  void lock() {}

  void unlock() {}
};

/**
 * @brief 读写互斥量 
 */
class RWMutex : Noncopyable
{
public:
  typedef ReadScopedLockImpl<RWMutex> ReadLock;

  typedef WriteScopedLockImpl<RWMutex> WriteLock;

  RWMutex()
  {
    pthread_rwlock_init(&lock_, nullptr);
  }

  ~RWMutex()
  {
    pthread_rwlock_destroy(&lock_);
  }

  void rdlock()
  {
    pthread_rwlock_rdlock(&lock_);
  }

  void wrlock()
  {
    pthread_rwlock_wrlock(&lock_);
  }

  void unlock()
  {
    pthread_rwlock_unlock(&lock_);
  }

private:
  pthread_rwlock_t lock_;
};

/**
 * @brief 空读写锁(用于调试)
 */
class NullRWMutex : Noncopyable {
public:
  typedef ReadScopedLockImpl<NullMutex> ReadLock;

  typedef WriteScopedLockImpl<NullMutex> WriteLock;

  /**
    * @brief 构造函数
    */
  NullRWMutex() {}
  /**
    * @brief 析构函数
    */
  ~NullRWMutex() {}

  /**
    * @brief 上读锁
    */
  void rdlock() {}

  /**
    * @brief 上写锁
    */
  void wrlock() {}
  /**
    * @brief 解锁
    */
  void unlock() {}
};

/**
 * @brief 自旋锁
 */
class Spinlock : Noncopyable
{
public:
  typedef ScopedLockImpl<Spinlock> Lock;

  /**
    * @brief 构造函数
    */
  Spinlock() {
      pthread_spin_init(&m_mutex, 0);
  }

  /**
    * @brief 析构函数
    */
  ~Spinlock() {
      pthread_spin_destroy(&m_mutex);
  }

  /**
    * @brief 上锁
    */
  void lock() {
      pthread_spin_lock(&m_mutex);
  }

  /**
    * @brief 解锁
    */
  void unlock() {
      pthread_spin_unlock(&m_mutex);
  }
private:
  pthread_spinlock_t m_mutex;

};

/**
 * @brief 原子锁 
 */
class CASLock : Noncopyable
{
public:
  typedef ScopedLockImpl<CASLock> Lock;

  CASLock()
  {
    mutex_.clear();
  }

  ~CASLock() {}

  void lock()
  {
    while(std::atomic_flag_test_and_set_explicit(&mutex_, std::memory_order_acquire));
  }

  void unlock()
  {
    std::atomic_flag_clear_explicit(&mutex_, std::memory_order_release);
  }
private:
  volatile std::atomic_flag mutex_;
};

class Scheduler;
// class FiberSemaphore : Noncopyable
// {
// public:
//   typedef Spinlock MutexType;

//   FiberSemaphore(size_t initial_concurrency = 0);
//   ~FiberSemaphore();
//   bool tryWait();
//   void wait();
//   void notify();

//   size_t getConcurrency() const { return concurrency_;}
//   void reset() { concurrency_ = 0;}
// private:
//   MutexType mutex_;
//   std::list<std::pair<Scheduler*, Fiber::ptr>> waiters_;
//   size_t concurrency_;
// }




} // namespace sylar

#endif