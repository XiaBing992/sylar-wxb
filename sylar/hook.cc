#include "hook.h"
#include "log.h"
#include "config.h"



namespace sylar {

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
  sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

bool is_hook_enable()
{
  return t_hook_enable;
}

void set_hook_enable(bool flag)
{
  t_hook_enable = flag;
}

} // namespace sylar