/*
 * @Author: XiaBing
 * @Date: 2023-12-25 09:59:03
 * @LastEditors: XiaBing
 * @LastEditTime: 2023-12-25 10:09:45
 * @FilePath: /sylar-wxb/sylar/singleton.h
 * @Description: 单例模式封装
 */

#ifndef SINGLETON_H
#define SINGLETON_H

#include <memory>

namespace sylar {

template<typename T, typename X, int N>
T& GetInstanceX()
{
  static T v;
  return v;
}

template<typename T, typename X, int N>
std::shared_ptr<T> GetInstancePtr()
{
  static std::shared_ptr<T> v(new T);
  return v;
}

/**
 * @brief 单例模式封装类
 * @details
 *  T 类型
 *  X 为了创造多个实例对应的Tag
 *  N 同一个Tag创造多个实例索引 
 */
template <typename T, typename X = void, int N = 0>
class Singleton
{
public:
  static T* GetInstance()
  {
    static T v;
    return &v;
  }
};

/**
 * @brief 单例模式智能指针封装类
 * @details 
 *  T 类型
 *  X 为了创造多个实例对应的Tag
 *  N 同一个Tag创造多个实例索引 
 */
template<typename T, typename X = void, int N = 0>
class SingletonPtr
{
public:
  static std::shared_ptr<T> GetInstance()
  {
    static std::shared_ptr<T> v(new T);
    return v;
  }
};

} // namespace sylar

#endif
