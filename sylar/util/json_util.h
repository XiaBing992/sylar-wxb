/*
 * @Author: XiaBing
 * @Date: 2024-01-29 14:27:17
 * @LastEditors: XiaBing
 * @LastEditTime: 2024-01-29 14:27:25
 * @FilePath: /sylar-wxb/sylar/json_util.h
 * @Description: 
 */
#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <string>
#include <json/json.h>

namespace sylar {

class JsonUtil
{
public:
  static bool NeedEscape(const std::string& v);

  static std::string Escape(const std::string& v);

  static std::string GetString(const Json::Value& json, const std::string& name,
                               const std::string& default_value = "");

  static double GetDouble(const Json::Value& json, const std::string& name,
                          double default_value = 0);
  
  static int32_t GetInt32(const Json::Value& json, const std::string& name,
                          int32_t default_value = 0);
  static uint32_t GetUint32(const Json::Value& json, const std::string& name,
                            uint32_t default_value = 0);
  static int64_t GetInt64(const Json::Value& json, const std::string& name ,
                          int64_t default_value = 0);
  static uint64_t GetUint64(const Json::Value& json, const std::string& name,
                            uint64_t default_value = 0);
  static bool FromString(Json::Value& json, const std::string& v);
  static std::string ToString(const Json::Value& json);

};

}


#endif