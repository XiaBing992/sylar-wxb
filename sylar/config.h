/*
 * @Author: XiaBing
 * @Date: 2023-12-27 10:01:38
 * @LastEditors: XiaBing
 * @LastEditTime: 2024-01-01 21:19:15
 * @FilePath: /sylar-wxb/sylar/config.h
 * @Description: 
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/parse.h"
#include "yaml-cpp/node/type.h"
#include <algorithm>
#include <bits/stdint-uintn.h>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
namespace sylar{

/**
 * @brief 配置变量的基类 
 */
class ConfigVarBase
{
public:
  typedef std::shared_ptr<ConfigVarBase> ptr;

  ConfigVarBase(const std::string& name, const std::string& description = "")
    : name_(name), description_(description)
  {
    std::transform(name_.begin(), name_.end(), name_.begin(), ::tolower);
  }

  virtual ~ConfigVarBase() {}

  const std::string& getName() const { return name_;}

  const std::string& getDescription() const {return description_;}

  virtual std::string toString() = 0;

  virtual bool fromString(const std::string& val) = 0;

  /**
   * @brief 配置参数值类型名称
   */  
  virtual std::string getTypeName() const = 0;

protected:
  std::string name_; // 配置参数的名称

  std::string description_; // 配置参数的描述
};

/**
 * @brief 类型转换模板 (source type, target type)
 */
template<typename F, typename T>
class LexicalCast
{
public:
  T operator()(const F& v)
  {
    return boost::lexical_cast<T>(v);
  }
};

/**
 * @brief 类型转换模板类偏特化（YAML String -> std::vector<T>) 
 */
template<typename T>
class LexicalCast<std::string, std::vector<T>>
{
public:
  std::vector<T> operator()(const std::string& v)
  {
    YAML::Node node = YAML::Load(v);
    typename std::vector<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); i++)
    {
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

/**
 * @brief 类型转换模板偏特化 
 */
template<typename T>
class LexicalCast<std::vector<T>, std::string>
{
public:
  std::string operator()(const std::vector<T>& v)
  {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto& i : v)
    {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

/**
 * @brief 类型转换模板偏特化 
 */
template<typename T>
class LexicalCast<std::string, std::list<T>>
{
public:
  std::list<T> operator()(const std::string& v)
  {
    YAML::Node node = YAML::Load(v);
    typename std::list<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); i++)
    {
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

template<typename T>
class LexicalCast<std::list<T>, std::string>
{
public:
std::string operator()(const std::list<T>& v)
{
  YAML::Node node(YAML::NodeType::Sequence);
  for (auto& i : v)
  {
    node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::set<T>)
 */
template<class T>
class LexicalCast<std::string, std::set<T> > {
public:
  std::set<T> operator()(const std::string& v) {
      YAML::Node node = YAML::Load(v);
      typename std::set<T> vec;
      std::stringstream ss;
      for(size_t i = 0; i < node.size(); ++i) {
          ss.str("");
          ss << node[i];
          vec.insert(LexicalCast<std::string, T>()(ss.str()));
      }
      return vec;
  }
};

/**
 * @brief 类型转换模板类片特化(std::set<T> 转换成 YAML String)
 */
template<class T>
class LexicalCast<std::set<T>, std::string> {
public:
  std::string operator()(const std::set<T>& v) {
      YAML::Node node(YAML::NodeType::Sequence);
      for(auto& i : v) {
          node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
      }
      std::stringstream ss;
      ss << node;
      return ss.str();
  }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_set<T>)
 */
template<class T>
class LexicalCast<std::string, std::unordered_set<T> > {
public:
  std::unordered_set<T> operator()(const std::string& v) {
      YAML::Node node = YAML::Load(v);
      typename std::unordered_set<T> vec;
      std::stringstream ss;
      for(size_t i = 0; i < node.size(); ++i) {
          ss.str("");
          ss << node[i];
          vec.insert(LexicalCast<std::string, T>()(ss.str()));
      }
      return vec;
  }
};

/**
 * @brief 类型转换模板类片特化(std::unordered_set<T> 转换成 YAML String)
 */
template<class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
  std::string operator()(const std::unordered_set<T>& v) {
      YAML::Node node(YAML::NodeType::Sequence);
      for(auto& i : v) {
          node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
      }
      std::stringstream ss;
      ss << node;
      return ss.str();
  }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::map<std::string, T>)
 */
template<class T>
class LexicalCast<std::string, std::map<std::string, T> > {
public:
  std::map<std::string, T> operator()(const std::string& v) {
      YAML::Node node = YAML::Load(v);
      typename std::map<std::string, T> vec;
      std::stringstream ss;
      for(auto it = node.begin();
              it != node.end(); ++it) {
          ss.str("");
          ss << it->second;
          vec.insert(std::make_pair(it->first.Scalar(),
                      LexicalCast<std::string, T>()(ss.str())));
      }
      return vec;
  }
};

/**
 * @brief 类型转换模板类片特化(std::map<std::string, T> 转换成 YAML String)
 */
template<class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
  std::string operator()(const std::map<std::string, T>& v) {
      YAML::Node node(YAML::NodeType::Map);
      for(auto& i : v) {
          node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
      }
      std::stringstream ss;
      ss << node;
      return ss.str();
  }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_map<std::string, T>)
 */
template<class T>
class LexicalCast<std::string, std::unordered_map<std::string, T> > {
public:
  std::unordered_map<std::string, T> operator()(const std::string& v) {
      YAML::Node node = YAML::Load(v);
      typename std::unordered_map<std::string, T> vec;
      std::stringstream ss;
      for(auto it = node.begin();
              it != node.end(); ++it) {
          ss.str("");
          ss << it->second;
          vec.insert(std::make_pair(it->first.Scalar(),
                      LexicalCast<std::string, T>()(ss.str())));
      }
      return vec;
  }
};

/**
 * @brief 类型转换模板类片特化(std::unordered_map<std::string, T> 转换成 YAML String)
 */
template<class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
  std::string operator()(const std::unordered_map<std::string, T>& v) {
      YAML::Node node(YAML::NodeType::Map);
      for(auto& i : v) {
          node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
      }
      std::stringstream ss;
      ss << node;
      return ss.str();
  }
};

/**
 * @brief 配置参数模板子类， 保存对应类型的参数值
 * @details T 参数的具体类型
 *          FromStr 从std::string转换成T类型的仿函数
 *          ToStr 从T转换成std::string的仿函数
 *          std::string 为YAML格式的字符串
 */
template<typename T, typename FromStr = LexicalCast<std::string, T>
       , typename ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase
{
public:
  // typedef RWMutex RWMutexType;
  typedef std::shared_ptr<ConfigVar> ptr;
  typedef std::function<void (const T& old_value, const T& new_value)> on_change_cb;

  /**
   * @brief: 
   * @param name 参数名称[0-9a-z_.]
   * @param default_value 参数默认值
   * @param description 参数的描述
   */  
  ConfigVar(const std::string& name, const T& default_value, const std::string& description = "") {}

  /**
   * @brief 参数值转换成YAML String 
   */  
  std::string toString() override
  {
    try 
    {
      // RWMutexType::ReadLock lock(mutex_);
      return ToStr(val_);
    } 
    catch (std::exception& e) 
    {
      // SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception "
      //           << e.what() << " convert: " << TypeToName<T>() << " to string"
      //           << " name=" << name_;
    }
  }

  /**
   * @brief 从YAML String 转换参数的值 
   */  
  bool fromString(const std::string& val) override
  {
    try
    {
      setValue(FromStr()(val));
    }
    catch (std::exception& e)
    {
      // SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception "
      //           << e.what() << " convert: string to " << TypeToName<T>()
      //           << " name=" << name_
      //           << " - " << val;
    }
    return false;
  }

  /**
   * @brief 获取当前参数的值 
   */  
  const T getValue()
  {
    // RWMutexType::ReadLock lock(mutex_);
    return val_;
  }

  void setValue(const T& v)
  {
    {
      //RWMutexType::ReadLock lock(mutex_);
      if (v == val_) return;

      for (auto& i : cbs_)
      {
        i.second(val_, v);
      }
    }

    // RWMutexType::WriteLock lock(mutex_);
    val_ = v;
  }

  // std::string getTypeName() const override { return getTypeName<T>();}

  /**
   * @brief 添加变化回调函数 
   * @return 返回该回调函数对应的唯一id，用于删除回调
   */  
  uint64_t addListener(on_change_cb cb)
  {
    static uint64_t s_fun_id = 0;
    // RWMutexType::WriteLock lock(mutex_);
    ++s_fun_id;
    cbs_[s_fun_id] = cb;
    return s_fun_id;
  }

  void delListener(uint64_t key)
  {
    // RWMutexType::WriteLock lock(mutex_);
    cbs_.erase(key);
  }

  /**
   * @brief 获取回调函数
   */  
  on_change_cb getListener(uint64_t key)
  {
    // RWMutexType::ReadLock lock(m_mutex);
    auto it = cbs_.find(key);
    return it == cbs_.end() ? nullptr : it->second;
  }

  void clearListener()
  {
    // RWMutexType::WriteLock lock(mutex_);
    cbs_.clear();
  }

private:
  // RWMutexType mutex_;
  T val_;

  std::map<std::uint64_t, on_change_cb> cbs_; // 变更回调函数组
};

/**
 * @brief ConfigVar的管理类
 * @details 提供便捷的方法创建/访问ConfigVar 
 */
class Config
{
public:
  typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
  // typedef RWMutex RWMutexType;

  template<typename T>
  static typename ConfigVar<T>::ptr Lookup(const std::string& name, const T& default_value
                                         , const std::string& description = "")
  {
    // RWMutexType::WriteLock lock(GetMutex());
    auto it = GetDatas().find(name);
    if (it != GetDatas().end())
    {
      auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
      if (tmp)
      {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists";
                return tmp;
      }
      else
      {

      }
    }
  }


private:
  /**
   * @brief 返回所有配置项 
   */  
  static ConfigVarMap&  GetDatas()
  {
    static ConfigVarMap s_datas;
    return s_datas;
  }

  // static RWMutexType& GetMutex()
  // {
  //   static RWMutexType s_mutex;
  //   return s_mutex;
  // }

  
};




} // namespace sylar

#endif