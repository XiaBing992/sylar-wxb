#ifndef ADDRESS_H
#define ADDRESS_H

#include <cstdint>
#include <memory>
#include <ostream>
#include <unistd.h>
#include <sys/socket.h>
#include <vector>
#include <map>
#include <netinet/in.h>
#include <sys/un.h>


namespace sylar {

class IPAddress;

/**
 * @brief 网络地址基类 
 */
class Address
{
public:
  typedef std::shared_ptr<Address> ptr;

  /**
   * @brief 通过socket指针创建Address
   */  
  static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);

  /**
   * @brief 通过host地址返回对应条件的所有address
   * @param host 域名、服务器名等  
   */  
  static bool Lookup(std::vector<Address::ptr>& result, const std::string& host, int family = AF_INET,
                      int type = 0, int protocol = 0);

  static Address::ptr LookupAny(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

  static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);

  /**
   * @brief 返回本机所有网卡的<网卡名 地址 子网掩码位数> 
   */  
  static bool GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t> >& result,
                    int family = AF_INET);

  /**
   * @brief 获取指定网卡的... 
   */  
  static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result
                    ,const std::string& iface, int family = AF_INET);

  
  virtual ~Address(){}

  int getFamily() const;

  virtual const sockaddr* getAddr() const = 0;

  virtual sockaddr* getAddr() = 0;

  virtual socklen_t getAddrLen() const = 0;

  /**
   * @brief 可读性输出地址 
   */  
  virtual std::ostream& insert(std::ostream& os) const = 0;

  /**
  * @brief 返回可读性字符串
  */
  std::string toString() const;

  bool operator<(const Address& rhs) const;

  bool operator==(const Address& rhs) const;

  bool operator!=(const Address& rhs) const;
};

/**
 * @brief IP地址的基类 
 */
class IPAddress : public Address
{
public:
  typedef std::shared_ptr<IPAddress> ptr;

  /**
   * @brief 通过域名、ip或者核武器名创建
   */  
  static IPAddress::ptr Create(const char* address, uint16_t port = 0);

  /**
   * @brief 获取该地址的广播地址
   * @param prefix_len 子网掩码位数
   */  
  virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

  /**
   * @brief 获取该地址网段
   */  
  virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;

  /**
   * @brief 获取子网掩码
   */  
  virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

  virtual uint32_t getPort() const = 0;

  virtual void setPort(uint16_t v) = 0;

};

class IPv4Address : public IPAddress
{
public:
  typedef std::shared_ptr<IPv4Address> ptr;

  /**
    * @brief 使用点分十进制地址创建IPv4Address
    */
  static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

  /**
    * @brief 通过sockaddr_in构造IPv4Address
    * @param[in] address sockaddr_in结构体
    */
  IPv4Address(const sockaddr_in& address);

  /**
    * @brief 通过二进制地址构造IPv4Address
    * @param[in] address 二进制地址address
    * @param[in] port 端口号
    */
  IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

  const sockaddr* getAddr() const override;
  sockaddr* getAddr() override;
  socklen_t getAddrLen() const override;
  std::ostream& insert(std::ostream& os) const override;

  IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
  IPAddress::ptr networdAddress(uint32_t prefix_len) override;
  IPAddress::ptr subnetMask(uint32_t prefix_len) override;
  uint32_t getPort() const override;
  void setPort(uint16_t v) override;
private:
  sockaddr_in addr_;
};

/**
 * @brief IPv6地址
 */
class IPv6Address : public IPAddress {
public:
  typedef std::shared_ptr<IPv6Address> ptr;
  /**
    * @brief 通过IPv6地址字符串构造IPv6Address
    */
  static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

  /**
    * @brief 无参构造函数
    */
  IPv6Address();

  /**
    * @brief 通过sockaddr_in6构造IPv6Address
    */
  IPv6Address(const sockaddr_in6& address);

  /**
    * @brief 通过IPv6二进制地址构造IPv6Address
    */
  IPv6Address(const uint8_t address[16], uint16_t port = 0);

  const sockaddr* getAddr() const override;
  sockaddr* getAddr() override;
  socklen_t getAddrLen() const override;
  std::ostream& insert(std::ostream& os) const override;

  IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
  IPAddress::ptr networdAddress(uint32_t prefix_len) override;
  IPAddress::ptr subnetMask(uint32_t prefix_len) override;
  uint32_t getPort() const override;
  void setPort(uint16_t v) override;
private:
  sockaddr_in6 addr_;
};

class UnixAddress : public Address {
public:
  typedef std::shared_ptr<UnixAddress> ptr;

  UnixAddress();

  /**
    * @brief 通过路径构造UnixAddress
    * @param path UnixSocket路径(长度小于UNIX_PATH_MAX)
    */
  UnixAddress(const std::string& path);

  const sockaddr* getAddr() const override;
  sockaddr* getAddr() override;
  socklen_t getAddrLen() const override;
  void setAddrLen(uint32_t v);
  std::string getPath() const;
  std::ostream& insert(std::ostream& os) const override;
private:
  sockaddr_un addr_;
  socklen_t length_;
};

/**
 * @brief 未知地址
 */
class UnknownAddress : public Address {
public:
  typedef std::shared_ptr<UnknownAddress> ptr;
  UnknownAddress(int family);
  UnknownAddress(const sockaddr& addr);
  const sockaddr* getAddr() const override;
  sockaddr* getAddr() override;
  socklen_t getAddrLen() const override;
  std::ostream& insert(std::ostream& os) const override;
private:
  sockaddr addr_;
};

std::ostream& operator<<(std::ostream& os, const Address& addr);

} // namespace sylar

#endif