/*
 * @Author: XiaBing
 * @Date: 2024-01-02 14:45:51
 * @LastEditors: XiaBing
 * @LastEditTime: 2024-01-11 10:31:39
 * @FilePath: /sylar-wxb/sylar/util.cpp
 * @Description: 
 */

#include <sys/syscall.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <string>

#include "util.h"

namespace sylar {

pid_t GetThreadId()
{
  return syscall(SYS_gettid);
}

void FSUtil::ListAllFile(std::vector<std::string>& files, const std::string& path, const std::string& subfix)
{
  if (access(path.c_str(), 0) != 0) return;

  DIR* dir = opendir(path.c_str());
  if (dir == nullptr) return;

  struct dirent* dp = nullptr;
  while((dp = readdir(dir)) != nullptr)
  {
    if (dp->d_type == DT_DIR) // 目录文件
    {
      if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
      {
        continue;
      }
      ListAllFile(files, path + "/" + dp->d_name, subfix);
    }
    else if (dp->d_type == DT_REG) // 普通文件
    {
      std::string filename(dp->d_name);
      if (subfix.empty())
      {
        files.push_back(path + "/" + filename);
      }
      else
      {
        if (filename.size() < subfix.size()) continue;
        if (filename.substr(filename.length() - subfix.size()) == subfix)
        {
          files.push_back(path + "/" + filename);
        }
      }
    }
  }
  closedir(dir);
}

} // namespace sylar