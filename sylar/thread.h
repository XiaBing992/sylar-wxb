/*
 * @Author: XiaBing
 * @Date: 2024-01-01 21:25:51
 * @LastEditors: XiaBing
 * @LastEditTime: 2024-01-30 09:59:58
 * @FilePath: /sylar-wxb/sylar/thread.h
 * @Description: 
 */
/*
 * @Author: XiaBing
 * @Date: 2024-01-01 21:25:51
 * @LastEditors: XiaBing
 * @LastEditTime: 2024-01-30 09:59:36
 * @FilePath: /sylar-wxb/sylar/thread.h
 * @Description: 
 */
#ifndef THREAD_H
#define THREAD_H

#include <ctime>
#include <functional>
#include <memory>

#include "noncopyable.h"
#include "mutex.h"


namespace sylar{

class Thread : Noncopyable
{
public:
  typedef std::shared_ptr<Thread> ptr;

  Thread(std::function<void()> cb, const std::string& name);

  ~Thread();

  pid_t getId() const { return id_;}

  const std::string& getName() { return name_;}

  void join();

  static Thread* GetThis();

  static const std::string& GetName();

  static void SetName(const std::string& name);

private:

  static void* run(void* arg);

  pid_t id_ = -1;

  pthread_t thread_ = 0;

  std::function<void()> cb_;

  std::string name_;

  Semaphore semaphore_;

}; 
} // namespace sylar
#endif