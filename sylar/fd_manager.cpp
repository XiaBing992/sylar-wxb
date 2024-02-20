/*
 * @Author: Xiabing
 * @Date: 2024-02-18 22:03:29
 * @LastEditors: Xiabing
 * @LastEditTime: 2024-02-18 22:20:14
 * @FilePath: /sylar-wxb/sylar/fd_manager.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by Xiabing, All Rights Reserved. 
 */

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fd_manager.h"
#include "hook.h"

namespace sylar {

FdCtx::FdCtx(int fd)
  :isInit_(false), isSocket_(false), sysNonblock_(false), userNonblock_(false), isClosed_(false), 
    fd_(fd), recvTimeout_(-1), sendTimeout_(-1) 
{
    init();
}

FdCtx::~FdCtx() 
{

}

bool FdCtx::init()
{
  if(isInit_)
  {
    return true;
  }
  recvTimeout_ = -1;
  sendTimeout_ = -1;

  struct stat fd_stat;
  if(-1 == fstat(fd_, &fd_stat))
  {
      isInit_ = false;
      isSocket_ = false;
  }
  else
  {
    isInit_ = true;
    isSocket_ = S_ISSOCK(fd_stat.st_mode);
  }

  if(isSocket_)
  {
    int flags = fcntl_f(fd_, F_GETFL, 0); // 获取文件状态
    if(!(flags & O_NONBLOCK)) {
        fcntl_f(fd_, F_SETFL, flags | O_NONBLOCK);
    }
    sysNonblock_ = true;
  }
  else
  {
    sysNonblock_ = false;
  }

  userNonblock_ = false;
  isClosed_ = false;
  return isInit_;
}

void FdCtx::setTimeout(int type, uint64_t v)
{
  if(type == SO_RCVTIMEO)
  {
    recvTimeout_ = v;
  }
  else
  {
    sendTimeout_ = v;
  }
}

uint64_t FdCtx::getTimeout(int type)
{
  if(type == SO_RCVTIMEO)
  {
    return recvTimeout_;
  }
  else
  {
    return sendTimeout_;
  }
}

FdManager::FdManager()
{
  datas_.resize(64);
}

FdCtx::ptr FdManager::get(int fd, bool auto_create)
{
  if(fd == -1) return nullptr;

  RWMutexType::ReadLock lock(mutex_);

  if((int)datas_.size() <= fd)
  {
    if(auto_create == false) return nullptr;
  }
  else
  {
    if(datas_[fd] || !auto_create)
    {
      return datas_[fd];
    }
  }
  lock.unlock();

  RWMutexType::WriteLock lock2(mutex_);
  FdCtx::ptr ctx(new FdCtx(fd));
  if(fd >= (int)datas_.size())
  {
    datas_.resize(fd * 1.5);
  }
  datas_[fd] = ctx;
  return ctx;
}

void FdManager::del(int fd)
{
  RWMutexType::WriteLock lock(mutex_);
  if((int)datas_.size() <= fd) return;
  datas_[fd].reset();
}

} // namespace sylar