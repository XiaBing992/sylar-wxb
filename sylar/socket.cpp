/*
 * @Author: Xiabing
 * @Date: 2024-02-18 22:03:29
 * @LastEditors: Xiabing
 * @LastEditTime: 2024-02-19 19:23:56
 * @FilePath: /sylar-wxb/sylar/socket.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by Xiabing, All Rights Reserved. 
 */
#include <netinet/tcp.h>

#include "socket.h"
#include "log.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "macro.h"
#include "hook.h"

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
  FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock_);
  if(ctx) {
      return ctx->getTimeout(SO_SNDTIMEO);
  }
  return -1;
}

void Socket::setSendTimeout(int64_t v)
{
  struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
  setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::getRecvTimeout()
{
  FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock_);
  if(ctx)
  {
    return ctx->getTimeout(SO_RCVTIMEO);
  }
  return -1;
}

void Socket::setRecvTimeout(int64_t v)
{
  struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
  setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::getOption(int level, int option, void* result, socklen_t* len)
{
  int rt = getsockopt(sock_, level, option, result, (socklen_t*)len);
  if(rt)
  {
    SYLAR_LOG_DEBUG(g_logger) << "getOption sock=" << sock_
        << " level=" << level << " option=" << option
        << " errno=" << errno << " errstr=" << strerror(errno);
    return false;
  }
  return true;
}

bool Socket::setOption(int level, int option, const void* result, socklen_t len)
{
  if(setsockopt(sock_, level, option, result, (socklen_t)len))
  {
    SYLAR_LOG_DEBUG(g_logger) << "setOption sock=" << sock_
        << " level=" << level << " option=" << option
        << " errno=" << errno << " errstr=" << strerror(errno);
    return false;
  }
  return true;
}

Socket::ptr Socket::accept()
{
  Socket::ptr sock(new Socket(family_, type_, protocol_));
  int newsock = ::accept(sock_, nullptr, nullptr);
  if(newsock == -1)
  {
    SYLAR_LOG_ERROR(g_logger) << "accept(" << sock_ << ") errno="
      << errno << " errstr=" << strerror(errno);
    return nullptr;
  }
  if(sock->init(newsock)) 
  {
    return sock;
  }
  return nullptr;
}

bool Socket::init(int sock)
{
  FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);
  if(ctx && ctx->isSocket() && !ctx->isClose())
  {
    sock_ = sock;
    isConnected_ = true;
    initSock();
    getLocalAddress();
    getRemoteAddress();
    return true;
  }
  return false;
}

bool Socket::bind(const Address::ptr addr)
{
  if(!isValid())
  {
    newSock();
    if(SYLAR_UNLIKELY(!isValid()))
    {
      return false;
    }
  }

  if(SYLAR_UNLIKELY(addr->getFamily() != family_))
  {
    SYLAR_LOG_ERROR(g_logger) << "bind sock.family("
      << family_ << ") addr.family(" << addr->getFamily()
      << ") not equal, addr=" << addr->toString();
    return false;
  }

  UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
  if(uaddr)
  {
    Socket::ptr sock = Socket::CreateUnixTCPSocket();
    if(sock->connect(uaddr))
    {
      return false;
    }
    else
    {
      sylar::FSUtil::Unlink(uaddr->getPath(), true); // 连接失败，将unix域通信的文件删除
    }
  }

  if(::bind(sock_, addr->getAddr(), addr->getAddrLen()))
  {
    SYLAR_LOG_ERROR(g_logger) << "bind error errrno=" << errno
        << " errstr=" << strerror(errno);
    return false;
  }
  getLocalAddress();
  return true;
}

bool Socket::reconnect(uint64_t timeout_ms)
{
  if(!remoteAddress_)
  {
    SYLAR_LOG_ERROR(g_logger) << "reconnect m_remoteAddress is null";
    return false;
  }
  localAddress_.reset();
  return connect(remoteAddress_, timeout_ms);
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms)
{
  remoteAddress_ = addr;
  if(!isValid())
  {
    newSock();
    if(SYLAR_UNLIKELY(!isValid()))
    {
      return false;
    }
  }

  if(SYLAR_UNLIKELY(addr->getFamily() != family_))
  {
    SYLAR_LOG_ERROR(g_logger) << "connect sock.family("
        << family_ << ") addr.family(" << addr->getFamily()
        << ") not equal, addr=" << addr->toString();
    return false;
  }

  if(timeout_ms == (uint64_t)-1)
  {
    if(::connect(sock_, addr->getAddr(), addr->getAddrLen()))
    {
      SYLAR_LOG_ERROR(g_logger) << "sock=" << sock_ << " connect(" << addr->toString()
          << ") error errno=" << errno << " errstr=" << strerror(errno);
      close();
      return false;
    }
  }
  else
  {
    if(::connect_with_timeout(sock_, addr->getAddr(), addr->getAddrLen(), timeout_ms))
    {
      SYLAR_LOG_ERROR(g_logger) << "sock=" << sock_ << " connect(" << addr->toString()
          << ") timeout=" << timeout_ms << " error errno="
          << errno << " errstr=" << strerror(errno);
      close();
      return false;
    }
  }
  isConnected_ = true;
  getRemoteAddress();
  getLocalAddress();
  return true;
}
bool Socket::listen(int backlog)
{
  if(!isValid())
  {
    SYLAR_LOG_ERROR(g_logger) << "listen error sock=-1";
    return false;
  }
  if(::listen(sock_, backlog))
  {
    SYLAR_LOG_ERROR(g_logger) << "listen error errno=" << errno
        << " errstr=" << strerror(errno);
    return false;
  }
  return true;
}

bool Socket::close()
{
  if(!isConnected_ && sock_ == -1)
  {
    return true;
  }
  isConnected_ = false;
  if(sock_ != -1)
  {
    ::close(sock_);
    sock_ = -1;
  }
  return false;
}

int Socket::send(const void* buffer, size_t length, int flags)
{
  if(isConnected()) return ::send(sock_, buffer, length, flags);

  return -1;
}

int Socket::send(const iovec* buffers, size_t length, int flags)
{
  if(isConnected())
  {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec*)buffers;
    msg.msg_iovlen = length;
    return ::sendmsg(sock_, &msg, flags);
  }
  return -1;
}

int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags) 
{
  if(isConnected())
  {
    return ::sendto(sock_, buffer, length, flags, to->getAddr(), to->getAddrLen());
  }
  return -1;
}

int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags)
{
  if(isConnected())
  {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec*)buffers;
    msg.msg_iovlen = length;
    msg.msg_name = to->getAddr();
    msg.msg_namelen = to->getAddrLen();
    return ::sendmsg(sock_, &msg, flags);
  }
  return -1;
}

int Socket::recv(void* buffer, size_t length, int flags)
{
  if(isConnected())
  {
    return ::recv(sock_, buffer, length, flags);
  }
  return -1;
}

int Socket::recv(iovec* buffers, size_t length, int flags)
{
  if(isConnected())
  {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec*)buffers;
    msg.msg_iovlen = length;
    return ::recvmsg(sock_, &msg, flags);
  }
  return -1;
}

int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags)
{
  if(isConnected())
  {
    socklen_t len = from->getAddrLen();
    return ::recvfrom(sock_, buffer, length, flags, from->getAddr(), &len);
  }
  return -1;
}

int Socket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags)
{
  if(isConnected())
  {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec*)buffers;
    msg.msg_iovlen = length;
    msg.msg_name = from->getAddr();
    msg.msg_namelen = from->getAddrLen();
    return ::recvmsg(sock_, &msg, flags);
  }
  return -1;
}

Address::ptr Socket::getRemoteAddress()
{
  if(remoteAddress_)
  {
    return remoteAddress_;
  }

  Address::ptr result;
  switch(family_)
  {
    case AF_INET:
      result.reset(new IPv4Address());
      break;
    case AF_INET6:
      result.reset(new IPv6Address());
      break;
    case AF_UNIX:
      result.reset(new UnixAddress());
      break;
    default:
      result.reset(new UnknownAddress(family_));
      break;
  }
  socklen_t addrlen = result->getAddrLen();
  if(getpeername(sock_, result->getAddr(), &addrlen))
  {
      //SYLAR_LOG_ERROR(g_logger) << "getpeername error sock=" << m_sock
      //    << " errno=" << errno << " errstr=" << strerror(errno);
      return Address::ptr(new UnknownAddress(family_));
  }
  if(family_ == AF_UNIX)
  {
    UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
    addr->setAddrLen(addrlen);
  }
  remoteAddress_ = result;
  return remoteAddress_;
}

Address::ptr Socket::getLocalAddress()
{
  if(localAddress_)
  {
    return localAddress_;
  }

  Address::ptr result;
  switch(family_)
  {
    case AF_INET:
      result.reset(new IPv4Address());
      break;
    case AF_INET6:
      result.reset(new IPv6Address());
      break;
    case AF_UNIX:
      result.reset(new UnixAddress());
      break;
    default:
      result.reset(new UnknownAddress(family_));
      break;
  }
  socklen_t addrlen = result->getAddrLen();
  if(getsockname(sock_, result->getAddr(), &addrlen))
  {
    SYLAR_LOG_ERROR(g_logger) << "getsockname error sock=" << sock_
        << " errno=" << errno << " errstr=" << strerror(errno);
    return Address::ptr(new UnknownAddress(family_));
  }
  if(family_ == AF_UNIX)
  {
    UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
    addr->setAddrLen(addrlen);
  }
  localAddress_ = result;
  return localAddress_;
}

bool Socket::isValid() const
{
  return sock_ != -1;
}

int Socket::getError()
{
  int error = 0;
  socklen_t len = sizeof(error);
  if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len))
  {
    error = errno;
  }
  return error;
}

std::ostream& Socket::dump(std::ostream& os) const
{
  os << "[Socket sock=" << sock_
    << " is_connected=" << isConnected_
    << " family=" << family_
    << " type=" << type_
    << " protocol=" << protocol_;
  if (localAddress_)
  {
      os << " local_address=" << localAddress_->toString();
  }
  if (remoteAddress_)
  {
      os << " remote_address=" << remoteAddress_->toString();
  }
  os << "]";
  return os;
}

std::string Socket::toString() const
{
  std::stringstream ss;
  dump(ss);
  return ss.str();
}

bool Socket::cancelRead()
{
  return IOManager::GetThis()->cancelEvent(sock_, sylar::IOManager::READ);
}

bool Socket::cancelWrite()
{
  return IOManager::GetThis()->cancelEvent(sock_, sylar::IOManager::WRITE);
}

bool Socket::cancelAccept()
{
  return IOManager::GetThis()->cancelEvent(sock_, sylar::IOManager::READ);
}

bool Socket::cancelAll()
{
  return IOManager::GetThis()->cancelAll(sock_);
}

void Socket::initSock()
{
  int val = 1;
  setOption(SOL_SOCKET, SO_REUSEADDR, val);
  if(type_ == SOCK_STREAM)
  {
    setOption(IPPROTO_TCP, TCP_NODELAY, val);
  }
}

void Socket::newSock()
{
  sock_ = socket(family_, type_, protocol_);
  if(SYLAR_LIKELY(sock_ != -1))
  {
    initSock();
  }
  else
  {
    SYLAR_LOG_ERROR(g_logger) << "socket(" << family_
        << ", " << type_ << ", " << protocol_ << ") errno="
        << errno << " errstr=" << strerror(errno);
  }
}

namespace {

struct _SSLInit
{
  _SSLInit()
  {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
  }
};

static _SSLInit s_init;

}

SSLSocket::SSLSocket(int family, int type, int protocol)
  :Socket(family, type, protocol) {}

Socket::ptr SSLSocket::accept()
{
  SSLSocket::ptr sock(new SSLSocket(family_, type_, protocol_));
  int newsock = ::accept(sock_, nullptr, nullptr);
  if(newsock == -1) {
      SYLAR_LOG_ERROR(g_logger) << "accept(" << sock_ << ") errno="
          << errno << " errstr=" << strerror(errno);
      return nullptr;
  }
  sock->ctx_ = ctx_;
  if(sock->init(newsock)) {
      return sock;
  }
  return nullptr;
}
bool SSLSocket::bind(const Address::ptr addr)
{
  return Socket::bind(addr);
}

bool SSLSocket::connect(const Address::ptr addr, uint64_t timeout_ms)
{
  bool v = Socket::connect(addr, timeout_ms);
  if(v)
  {
    ctx_.reset(SSL_CTX_new(SSLv23_client_method()), SSL_CTX_free);
    ssl_.reset(SSL_new(ctx_.get()),  SSL_free);
    SSL_set_fd(ssl_.get(), sock_);
    v = (SSL_connect(ssl_.get()) == 1);
  }
  return v;
}

bool SSLSocket::listen(int backlog)
{
  return Socket::listen(backlog);
}

bool SSLSocket::close()
{
  return Socket::close();
}

int SSLSocket::send(const void* buffer, size_t length, int flags)
{
  if(ssl_)
  {
    return SSL_write(ssl_.get(), buffer, length);
  }
  return -1;
}

int SSLSocket::send(const iovec* buffers, size_t length, int flags)
{
  if (!ssl_) return -1;
  int total = 0;
  for(size_t i = 0; i < length; ++i)
  {
    int tmp = SSL_write(ssl_.get(), buffers[i].iov_base, buffers[i].iov_len);
    if(tmp <= 0) return tmp;
    
    total += tmp;
    if(tmp != (int)buffers[i].iov_len) break;

  }
  return total;
}

int SSLSocket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags)
{
  SYLAR_ASSERT(false);
  return -1;
}

int SSLSocket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags)
{
  SYLAR_ASSERT(false);
  return -1;
}

int SSLSocket::recv(void* buffer, size_t length, int flags)
{
  if(ssl_)
  {
    return SSL_read(ssl_.get(), buffer, length);
  }
  return -1;
}

int SSLSocket::recv(iovec* buffers, size_t length, int flags)
{
  if(!ssl_) return -1;
  
  int total = 0;
  for(size_t i = 0; i < length; ++i)
  {
    int tmp = SSL_read(ssl_.get(), buffers[i].iov_base, buffers[i].iov_len);
    if(tmp <= 0)
    {
      return tmp;
    }
    total += tmp;
    if(tmp != (int)buffers[i].iov_len)
    {
      break;
    }
  }
  return total;
}

int SSLSocket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags)
{
  SYLAR_ASSERT(false);
  return -1;
}

int SSLSocket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags)
{
  SYLAR_ASSERT(false);
  return -1;
}

bool SSLSocket::init(int sock)
{
  bool v = Socket::init(sock);
  if(v)
  {
    ssl_.reset(SSL_new(ctx_.get()),  SSL_free);
    SSL_set_fd(ssl_.get(), sock_);
    v = (SSL_accept(ssl_.get()) == 1);
  }
  return v;
}

bool SSLSocket::loadCertificates(const std::string& cert_file, const std::string& key_file)
{
  ctx_.reset(SSL_CTX_new(SSLv23_server_method()), SSL_CTX_free);
  if(SSL_CTX_use_certificate_chain_file(ctx_.get(), cert_file.c_str()) != 1) {
      SYLAR_LOG_ERROR(g_logger) << "SSL_CTX_use_certificate_chain_file("
          << cert_file << ") error";
      return false;
  }
  if(SSL_CTX_use_PrivateKey_file(ctx_.get(), key_file.c_str(), SSL_FILETYPE_PEM) != 1) {
      SYLAR_LOG_ERROR(g_logger) << "SSL_CTX_use_PrivateKey_file("
          << key_file << ") error";
      return false;
  }
  if(SSL_CTX_check_private_key(ctx_.get()) != 1) {
      SYLAR_LOG_ERROR(g_logger) << "SSL_CTX_check_private_key cert_file="
          << cert_file << " key_file=" << key_file;
      return false;
  }
  return true;
}

SSLSocket::ptr SSLSocket::CreateTCP(sylar::Address::ptr address)
{
  SSLSocket::ptr sock(new SSLSocket(address->getFamily(), TCP, 0));
  return sock;
}

SSLSocket::ptr SSLSocket::CreateTCPSocket()
{
  SSLSocket::ptr sock(new SSLSocket(IPv4, TCP, 0));
  return sock;
}

SSLSocket::ptr SSLSocket::CreateTCPSocket6()
{
  SSLSocket::ptr sock(new SSLSocket(IPv6, TCP, 0));
  return sock;
}

std::ostream& SSLSocket::dump(std::ostream& os) const
{
  os << "[SSLSocket sock=" << sock_
      << " is_connected=" << isConnected_
      << " family=" << family_
      << " type=" << type_
      << " protocol=" << protocol_;
  if(localAddress_)
  {
    os << " local_address=" << localAddress_->toString();
  }
  if(remoteAddress_)
  {
    os << " remote_address=" << remoteAddress_->toString();
  }
  os << "]";
  return os;
}

std::ostream& operator<<(std::ostream& os, const Socket& sock)
{
  return sock.dump(os);
}

} // namespace sylar