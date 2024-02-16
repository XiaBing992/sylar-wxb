#include <unistd.h>
#include <sys/stat.h>

#include "fd_manager.h"

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
    int flags = fcntl_f(fd_, F_GETFL, 0);
    if(!(flags & O_NONBLOCK)) {
        fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
    }
    m_sysNonblock = true;
  }
  else
  {
    m_sysNonblock = false;
  }

  m_userNonblock = false;
  m_isClosed = false;
  return m_isInit;
}

} // namespace sylar