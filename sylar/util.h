/*
 * @Author: XiaBing
 * @Date: 2024-01-02 14:45:44
 * @LastEditors: XiaBing
 * @LastEditTime: 2024-01-02 14:56:38
 * @FilePath: /sylar-wxb/sylar/util.h
 * @Description: 
 */

#ifndef UTIL_H
#define UTIL_H

#include <thread.h>

namespace sylar
{
  pid_t GetThreadId();

  uint32_t GetFiberId();
}


#endif
