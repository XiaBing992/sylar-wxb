#include "log.h"
#include "iomanager.h"
#include "address.h"
#include "socket.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();


void test2()
{
  sylar::IPAddress::ptr addr = sylar::Address::LookupAnyIPAddress("www.baidu.com:80");
  if(addr)
  {
    SYLAR_LOG_INFO(g_logger) << "get address: " << addr->toString();
  } 
  else
  {
    SYLAR_LOG_ERROR(g_logger) << "get address fail";
    return;
  }

  sylar::Socket::ptr sock = sylar::Socket::CreateTCP(addr);
  if(!sock->connect(addr)) {
      SYLAR_LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
      return;
  } else {
      SYLAR_LOG_INFO(g_looger) << "connect " << addr->toString() << " connected";
  }

  uint64_t ts = sylar::GetCurrentUS();
  for(size_t i = 0; i < 10000000000ul; ++i) {
      if(int err = sock->getError()) {
          SYLAR_LOG_INFO(g_looger) << "err=" << err << " errstr=" << strerror(err);
          break;
      }

      //struct tcp_info tcp_info;
      //if(!sock->getOption(IPPROTO_TCP, TCP_INFO, tcp_info)) {
      //    SYLAR_LOG_INFO(g_looger) << "err";
      //    break;
      //}
      //if(tcp_info.tcpi_state != TCP_ESTABLISHED) {
      //    SYLAR_LOG_INFO(g_looger)
      //            << " state=" << (int)tcp_info.tcpi_state;
      //    break;
      //}
      static int batch = 10000000;
      if(i && (i % batch) == 0) {
          uint64_t ts2 = sylar::GetCurrentUS();
          SYLAR_LOG_INFO(g_looger) << "i=" << i << " used: " << ((ts2 - ts) * 1.0 / batch) << " us";
          ts = ts2;
      }
  }
}

int main()
{
  sylar::IOManager iom;
  iom.schedule(&test2);
  return 0;
}