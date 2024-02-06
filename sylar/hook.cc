#include <cstdint>
#include <dlfcn.h>

#include "hook.h"
#include "log.h"
#include "config.h"



namespace sylar {

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
  sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

#define HOOK_FUNC(XX) \
  XX(sleep) \
  XX(usleep) \
  XX(nanosleep) \
  XX(socket) \
  XX(connect) \
  XX(accept) \
  XX(read) \
  XX(readv) \
  XX(recv) \
  XX(recvfrom) \
  XX(recvmsg) \
  XX(write) \
  XX(writev) \
  XX(send) \
  XX(sendto) \
  XX(sendmsg) \
  XX(close) \
  XX(fcntl) \
  XX(ioctl) \
  XX(getsockopt) \
  XX(setsockopt)

void hook_init()
{
  static bool is_inited = false;
  if (is_inited) return;

#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name); // 返回下一个共享库中名为name的函数指针
  HOOK_FUNC(XX);
#undef XX
}

static uint64_t s_connnect_timeout = -1;
struct _HookIniter
{
  _HookIniter()
  {
    hook_init();
    s_connnect_timeout = g_tcp_connect_timeout->getValue();

    g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value)
    {
      SYLAR_LOG_INFO(g_logger) << "tcp connect timeout changed from"
        << old_value << " to" << new_value;
      s_connnect_timeout = new_value;
    });
  }
  
};

static _HookIniter s_hook_initer;

bool is_hook_enable()
{
  return t_hook_enable;
}

void set_hook_enable(bool flag)
{
  t_hook_enable = flag;
}

} // namespace sylar

struct timer_info
{
  int cancelled = 0;
};

