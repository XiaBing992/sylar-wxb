/*
 * @Author: XiaBing
 * @Date: 2024-01-01 21:23:12
 * @LastEditors: XiaBing
 * @LastEditTime: 2024-01-01 21:23:22
 * @FilePath: /sylar-wxb/sylar/noncopyable.h
 * @Description: 
 */

 #ifndef NONCOPYABLE_H
 #define NONCOPYABLE_H

namespace sylar{
class Noncopyable
{
public:
  Noncopyable() = default;

  ~Noncopyable() = default;

  Noncopyable(const Noncopyable&) = delete;

  Noncopyable& operator=(const Noncopyable&) = delete;
};

} // namespace sylar

 #endif
