#include "socket.h"
#include "log.h"
#include "iomanager.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Socket::ptr Socket::CreateTCP(sylar::Address::ptr address)
{
  Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
  return sock;
}

Socket::ptr Socket::CreateUDP(sylar::Address::ptr address)
{
  Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
  sock->newSock();
  sock->isConnected_ = true;
  return sock;
}

Socket::ptr Socket::CreateTCPSocket()
{
  Socket::ptr sock(new Socket(IPv4, TCP, 0));
  return sock;
}

Socket::ptr Socket::CreateUDPSocket()
{
  Socket::ptr sock(new Socket(IPv4, UDP, 0));
  sock->newSock();
  sock->isConnected_ = true;
  return sock;
}

Socket::ptr Socket::CreateTCPSocket6()
{
  Socket::ptr sock(new Socket(IPv6, TCP, 0));
  return sock;
}

Socket::ptr Socket::CreateUDPSocket6()
{
  Socket::ptr sock(new Socket(IPv6, UDP, 0));
  sock->newSock();
  sock->isConnected_ = true;
  return sock;
}

Socket::ptr Socket::CreateUnixTCPSocket()
{
  Socket::ptr sock(new Socket(UNIX, TCP, 0));
  return sock;
}

Socket::ptr Socket::CreateUnixUDPSocket()
{
  Socket::ptr sock(new Socket(UNIX, UDP, 0));
  return sock;
}

Socket::Socket(int family, int type, int protocol)
    :sock_(-1)
    ,family_(family)
    ,type_(type)
    ,protocol_(protocol)
    ,isConnected_(false) {
}
Socket::~Socket()
{
  close();
}

int64_t Socket::getSendTimeout()
{
  FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
  if(ctx) {
      return ctx->getTimeout(SO_SNDTIMEO);
  }
  return -1;
}

} // namespace sylar