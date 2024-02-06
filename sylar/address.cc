#include <cstdint>
#include <ifaddrs.h>
#include <memory>
#include <sstream>
#include <sys/socket.h>
#include <utility>
#include <vector>
#include <netdb.h>
#include <string>

#include "address.h"
#include "endian.h"
#include "log.h"
#include "endian.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

/**
 * @brief 创造指定掩码位数(指定0的个数)
 */
template<typename T>
static T CreateMask(uint32_t bits)
{
  return (1 << (sizeof(T) * 8 - bits)) - 1;
}

template<typename T>
static uint32_t CountBytes(T value)
{
  uint32_t result = 0;
  for (; value; result++)
  {
    value &= value - 1;
  }

  return result;
}

Address::ptr Address::LookupAny(const std::string& host, int family, int type, int protocol)
{
  std::vector<Address::ptr> result;
  if (Lookup(result, host, family, type, protocol)) return result[0];

  return nullptr;
}

IPAddress::ptr Address::LookupAnyIPAddress(const std::string& host, int family, int type, int protocol)
{
  std::vector<Address::ptr> result;
  if (Lookup(result, host, family, type, protocol))
  {
    for (auto& i : result)
    {
      IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
      if (v) return v;
    }
  }
  
  return nullptr;
}

bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host, int family,
                      int type, int protocol) 
{
  addrinfo hints, *results, *next; // 用于解析hostname
  hints.ai_flags = 0;
  hints.ai_family = family;
  hints.ai_socktype = type;
  hints.ai_protocol = protocol;
  hints.ai_addrlen = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  std::string node;
  const char* service = NULL;

  //检查 ipv6address serivce
  if(!host.empty() && host[0] == '[') 
  {
    const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
    if(endipv6) 
    {
      //TODO check out of range
      if(*(endipv6 + 1) == ':') 
      {
          service = endipv6 + 2;
      }
      node = host.substr(1, endipv6 - host.c_str() - 1);
    }
  }

  //检查 node serivce
  if(node.empty()) 
  {
    service = (const char*)memchr(host.c_str(), ':', host.size());
    if(service) 
    {
      if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) 
      {
        node = host.substr(0, service - host.c_str());
        ++service;
      }
    }
  }

  if(node.empty()) 
  {
    node = host;
  }
  int error = getaddrinfo(node.c_str(), service, &hints, &results);
  if(error) 
  {
    SYLAR_LOG_DEBUG(g_logger) << "Address::Lookup getaddress(" << host << ", "
        << family << ", " << type << ") err=" << error << " errstr="
        << gai_strerror(error);
    return false;
  }

  next = results;
  while(next) 
  {
    result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
    //SYLAR_LOG_INFO(g_logger) << ((sockaddr_in*)next->ai_addr)->sin_addr.s_addr;
    next = next->ai_next;
  }

  freeaddrinfo(results);
  return !result.empty();
}

bool Address::GetInterfaceAddresses(std::multimap<std::string ,std::pair<Address::ptr, uint32_t> >& result,
                                    int family)
{
  struct ifaddrs* next, *results; // 存储网络接口信息
  if (getifaddrs(&results) != 0)
  {
    SYLAR_LOG_DEBUG(g_logger) << "Address::GetInterfaceAddresses getifaddrs " " err=" << errno << " errstr=" << strerror(errno);
    return false;
  }

  try {
    for (next = results; next; next = next->ifa_next)
    {
      Address::ptr addr;
      uint32_t prefix_len = ~0u;

      if (family != AF_UNSPEC && family != next->ifa_addr->sa_family) continue;

      switch (next->ifa_addr->sa_family) 
      {
        case AF_INET:
        {
          addr = Create(next->ifa_addr, sizeof(sockaddr_in));
          uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
          prefix_len = CountBytes(netmask);
          break;
        }
        case AF_INET6:
        {
          addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
          in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
          prefix_len = 0;
          for(int i = 0; i < 16; ++i) {
              prefix_len += CountBytes(netmask.s6_addr[i]);
          }
          break;
        }
        default:
          break;
      }

      if (addr)
      {
        result.insert(std::make_pair(next->ifa_name, std::make_pair(addr, prefix_len)));
      }
    }
  } catch (...)
  {
    SYLAR_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
    freeifaddrs(results);
    return false;
  }
  freeifaddrs(results);
  return !result.empty();
}

bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >& result
                    ,const std::string& iface, int family) 
{
  if(iface.empty() || iface == "*") 
  {
    if(family == AF_INET || family == AF_UNSPEC) 
    {
      result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
    }
    if(family == AF_INET6 || family == AF_UNSPEC) 
    {
      result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
    }
    return true;
  }

  std::multimap<std::string ,std::pair<Address::ptr, uint32_t> > results;

  if(!GetInterfaceAddresses(results, family)) 
  {
    return false;
  }

  auto its = results.equal_range(iface);
  for(; its.first != its.second; ++its.first) 
  {
    result.push_back(its.first->second);
  }
  return !result.empty();
}

int Address::getFamily() const
{
  return getAddr()->sa_family;
}
std::string Address::toString() const
{
  std::stringstream ss;
  insert(ss);
  return ss.str();
}

Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen)
{
  if(addr == nullptr) 
  {
    return nullptr;
  }

  Address::ptr result;
  switch(addr->sa_family) 
  {
    case AF_INET:
      result.reset(new IPv4Address(*(const sockaddr_in*)addr));
      break;
    case AF_INET6:
      result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
      break;
    default:
      result.reset(new UnknownAddress(*addr));
      break;
  }
  return result;
}

bool Address::operator<(const Address& rhs) const 
{
  socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
  int result = memcmp(getAddr(), rhs.getAddr(), minlen);
  if(result < 0) 
  {
    return true;
  } 
  else if(result > 0) 
  {
    return false;
  } 
  else if(getAddrLen() < rhs.getAddrLen()) 
  {
    return true;
  }
  return false;
}

bool Address::operator==(const Address& rhs) const 
{
  return getAddrLen() == rhs.getAddrLen() && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address& rhs) const
{
  return !(*this == rhs);
}

IPAddress::ptr IPAddress::Create(const char* address, uint16_t port)
{
  addrinfo hints, *results;
  memset(&hints, 0, sizeof(addrinfo));

  hints.ai_flags = AI_NUMERICHOST; // 标识纯数字主机地址
  hints.ai_family = AF_UNSPEC;

  int error = getaddrinfo(address, NULL, &hints, &results);
  if(error) 
  {
    SYLAR_LOG_DEBUG(g_logger) << "IPAddress::Create(" << address
        << ", " << port << ") error=" << error
        << " errno=" << errno << " errstr=" << strerror(errno);
    return nullptr;
  }

  try {
    IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                              Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
    if (result) result->setPort(port);
    freeaddrinfo(results);
    return result;
  } catch (...) {
    freeaddrinfo(results);
    return nullptr;
  }

}

IPv4Address::ptr IPv4Address::Create(const char *address, uint16_t port)
{
  IPv4Address::ptr rt(new IPv4Address);
  rt->addr_.sin_port = byteswapOnLittleEndian(port);
}

} // namespce sylar