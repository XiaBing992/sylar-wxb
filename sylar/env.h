/*
 * @Author: XiaBing
 * @Date: 2024-01-03 22:42:32
 * @LastEditors: XiaBing
 * @LastEditTime: 2024-01-03 23:11:59
 * @FilePath: /sylar-wxb/sylar/env.h
 * @Description: 
 */
#ifndef ENV_H
#define ENV_H
#include <map>
#include <vector>

#include "mutex.h"
#include "singleton.h"

namespace sylar {

class Env
{
public:
  typedef RWMutex RWMutexType;
  bool init(int argc, char** argv);

  void add(const std::string& key, const std::string& val);
  bool has(const std::string& key);
  void del(const std::string& key);
  std::string get(const std::string& key, const std::string& default_value = "");

  void addHelp(const std::string& key, const std::string& desc);
  void removeHelp(const std::string& key);
  void printHelp();

  const std::string& getExe() const { return exe_;}
  const std::string& getCwd() const { return cwd_;}

  bool setEnv(const std::string& key, const std::string& val);
  std::string getEnv(const std::string& key, const std::string& default_value = "");

  std::string getAbsolutePath(const std::string& path) const;
  std::string getAbsoluteWorkPath(const std::string& path) const;
  std::string getConfigPath();
  
private:
  RWMutexType mutex_;

  std::map<std::string, std::string> args_;

  std::vector<std::pair<std::string, std::string>> helps_;

  std::string program_;

  std::string exe_;

  std::string cwd_;
};

typedef sylar::Singleton<Env> EnvMgr;
} // namespace sylar

#endif