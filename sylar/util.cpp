/*
 * @Author: XiaBing
 * @Date: 2024-01-02 14:45:51
 * @LastEditors: XiaBing
 * @LastEditTime: 2024-01-02 14:47:43
 * @FilePath: /sylar-wxb/sylar/util.cpp
 * @Description: 
 */

#include <sys/syscall.h>
#include <unistd.h>
#include "util.h"

namespace sylar {
pid_t GetThreadId()
{
  return syscall(SYS_gettid);
}

} // namespace sylar