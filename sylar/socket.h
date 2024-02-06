/*
 * @Author: Xiabing
 * @Date: 2024-02-05 10:50:42
 * @LastEditors: WXB 1763567512@qq.com
 * @LastEditTime: 2024-02-05 19:34:54
 * @FilePath: /sylar-wxb/sylar/socket.h
 * @Description: socket封装
 * 
 * Copyright (c) 2024 by Xiabing, All Rights Reserved. 
 */

#ifndef SOCKET_H
#define SOCKET_H

#include <memory>
#include <netinet/in.h>
#include <type_traits>
#include <sys/socket.h>
#include "address.h"
#include "noncopyable.h"

namespace sylar {

class Address;

class Socket : public std::enable_shared_from_this<Socket>, Noncopyable
{
public:
  typedef std::shared_ptr<Socket> ptr;
  typedef std::weak_ptr<Socket> weak_ptr;

  /**
   * @brief Socket类型 
   */  
  enum Type
  {
    TCP = SOCK_STREAM,
    UDP = SOCK_DGRAM
  };

  enum Family
  {
    IPv4 = AF_INET,
    IPV6 = AF_INET6,
    UNIX = AF_UNIX // 本地套接字
  };

  static Socket::ptr CreateTCP(sylar::Address::ptr address);

};

} // namespace sylar

 #endif
