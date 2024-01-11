/*
 * @Author: XiaBing
 * @Date: 2024-01-02 14:45:44
 * @LastEditors: XiaBing
 * @LastEditTime: 2024-01-11 10:49:23
 * @FilePath: /sylar-wxb/sylar/util.h
 * @Description: 
 */

#ifndef UTIL_H
#define UTIL_H

#include <thread.h>
#include <vector>
#include <cxxabi.h>

namespace sylar
{

pid_t GetThreadId();

uint32_t GetFiberId();

template<class T>
const char* TypeToName() {
    static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    return s_name;
}


class FSUtil 
{
public:
  /**
   * @brief 将路径下所有文件加入files
   * @param files 用于存储所有文件
   * @param path 路径
   * @param subfix 用于选定满足特定前缀的文件 
   */  
  static void ListAllFile(std::vector<std::string>& files, const std::string& path,
                          const std::string& subfix);
};
  
}


#endif
