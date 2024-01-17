/*
 * @Author: XiaBing
 * @Date: 2024-01-02 14:45:44
 * @LastEditors: XiaBing
 * @LastEditTime: 2024-01-17 15:32:46
 * @FilePath: /sylar-wxb/sylar/util.h
 * @Description: 
 */

#ifndef UTIL_H
#define UTIL_H

#include <fstream>
#include <ios>
#include <ostream>
#include <thread.h>
#include <vector>
#include <cxxabi.h>

namespace sylar
{

pid_t GetThreadId();

uint32_t GetFiberId();

void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);

/**
 * @brief 获取当前栈信息的字符串 
 * @param size 栈的最大层数
 * @param skip 跳过栈顶的层数
 * @param prefix 栈信息输出的内容 
 */
std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");

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
  static bool Mkdir(const std::string& dirname);
  static bool IsRunningPidfile(const std::string& pidfile);
  static bool Rm(const std::string& path);
  static bool Mv(const std::string& from, const std::string& to);
  static bool Realpath(const std::string& path, std::string& rpath);
  static bool Symlink(const std::string& frm, const std::string& to);
  static bool Unlink(const std::string& filename, bool exist = false);
  static std::string Dirname(const std::string& filename);
  static std::string Basename(const std::string& filename);
  static bool OpenForRead(std::ifstream& ifs, const std::string& filename, std::ios_base::openmode mode);
  static bool OpenForWrite(std::ofstream& ofs, const std::string& filename, std::ios_base::openmode mode);
};
  
}


#endif
