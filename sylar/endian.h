/*
 * @Author: Xiabing
 * @Date: 2024-02-05 18:23:31
 * @LastEditors: WXB 1763567512@qq.com
 * @LastEditTime: 2024-02-05 20:14:42
 * @FilePath: /sylar-wxb/sylar/endian.h
 * @Description: 
 * 
 * Copyright (c) 2024 by Xiabing, All Rights Reserved. 
 */
#ifndef ENDIAN_H
#define ENDIAN_H

#include <cstdint>
#include <byteswap.h>
#include <stdint.h>
#include <type_traits>

#define SYLAR_LITTLE_ENDIAN 1
#define SYLAR_BIG_ENDIAN 2



namespace sylar {

/**
 * @brief 8字节类型 
 */
template<typename T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value)
{
  return (T)bswap_64((uint64_t)value);
}

/**
 * @brief 4字节类型的字节序转化
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) 
{
  return (T)bswap_32((uint32_t)value);
}

/**
 * @brief 2字节类型的字节序转化
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) 
{
  return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN // 大端序
  #define SYLAR_BYTE_ORDER SYLAR_BIG_ENDIAN
#endif

#if SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN

template<class T>
T byteswapOnLittleEndian(T t) 
{
  return t;
}

template<class T>
T byteswapOnBigEndian(T t) 
{
  return byteswap(t);
}

#endif


} // namespace sylar


#endif