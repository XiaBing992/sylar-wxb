/*
 * @Author: XiaBing
 * @Date: 2024-01-02 14:45:44
 * @LastEditors: XiaBing
 * @LastEditTime: 2024-01-29 15:00:59
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

#include <yaml-cpp/yaml.h>
#include <json/json.h>
#include <boost/lexical_cast.hpp>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>


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

template<typename V, typename Map, typename K>
V GetParamValue(const Map& m, const K& k, const V& def = V())
{
  auto it = m.find(k);
  if (it == m.end()) return def;

  try {
    return boost::lexical_cast<V>(it->second);
  } catch (...){

  }

  return def;
}

template<typename V, typename Map, typename K>
bool CheckGetParamValue(const Map& m, const K& k, V& v)
{
  auto it = m.find(k);
  if (it == m.end()) return false;

  try {
    v = boost::lexical_cast<V>(it->second);
    return true;
  } catch (...) {

  }

  return false;
}

class TypeUtil
{
public:
  static int8_t ToChar(const std::string& str);
  static int64_t Atoi(const std::string& str);
  static double Atof(const std::string& str);
  static int8_t ToChar(const char* str);
  static int64_t Atoi(const char* str);
  static double Atof(const char* str);
};

/**
 * @brief 原子操作 
 */
class Atomic
{
public:
  template<typename T, typename S = T>
  static T addFetch(volatile T& t, S v = 1)
  {
    return __sync_add_and_fetch(&t, (T)v);
  }

  template<class T, class S = T>
  static T subFetch(volatile T& t, S v = 1) {
      return __sync_sub_and_fetch(&t, (T)v);
  }

  template<class T, class S>
  static T orFetch(volatile T& t, S v) {
      return __sync_or_and_fetch(&t, (T)v);
  }

  template<class T, class S>
  static T andFetch(volatile T& t, S v) {
      return __sync_and_and_fetch(&t, (T)v);
  }

  template<class T, class S>
  static T xorFetch(volatile T& t, S v) {
      return __sync_xor_and_fetch(&t, (T)v);
  }

  template<class T, class S>
  static T nandFetch(volatile T& t, S v) {
      return __sync_nand_and_fetch(&t, (T)v);
  }

  template<class T, class S>
  static T fetchAdd(volatile T& t, S v = 1) {
      return __sync_fetch_and_add(&t, (T)v);
  }

  template<class T, class S>
  static T fetchSub(volatile T& t, S v = 1) {
      return __sync_fetch_and_sub(&t, (T)v);
  }

  template<class T, class S>
  static T fetchOr(volatile T& t, S v) {
      return __sync_fetch_and_or(&t, (T)v);
  }

  template<class T, class S>
  static T fetchAnd(volatile T& t, S v) {
      return __sync_fetch_and_and(&t, (T)v);
  }

  template<class T, class S>
  static T fetchXor(volatile T& t, S v) {
      return __sync_fetch_and_xor(&t, (T)v);
  }

  template<class T, class S>
  static T fetchNand(volatile T& t, S v) {
      return __sync_fetch_and_nand(&t, (T)v);
  }

  template<class T, class S>
  static T compareAndSwap(volatile T& t, S old_val, S new_val) {
      return __sync_val_compare_and_swap(&t, (T)old_val, (T)new_val);
  }

  template<class T, class S>
  static bool compareAndSwapBool(volatile T& t, S old_val, S new_val) {
      return __sync_bool_compare_and_swap(&t, (T)old_val, (T)new_val);
  }
};

template<typename T>
void nop(T*) {}

template<typename T>
void delete_array(T* v)
{
  if (v) delete []v;
}

template<typename T>
class SharedArray
{
public:
  explicit SharedArray(const uint64_t& size = 0, T* p = 0)
    : size_(size), ptr_(p, delete_array<T>) {}

  // template<typename D>
  // sharedArray(const uint64_t& size, T* p, D d)
  //   : size_(size), ptr_(p, d) {};
  
  SharedArray(const SharedArray& r)
    : size_(r.size_), ptr_(r.ptr_) {}

  SharedArray& operator=(const SharedArray& r)
  {
    size_ = r.size_;
    ptr_ = r.ptr_;
    return *this;
  }

  T& operator[](std::ptrdiff_t i) const 
  {
    return ptr_.get()[i];
  }

  T* get() const {
      return ptr_.get();
  }

  bool unique() const
  {
    return ptr_.unique();
  }

  long use_count() const 
  {
    return ptr_.use_count();
  }

  void swap(SharedArray& b) 
  {
    std::swap(size_, b.size_);
    ptr_.swap(b.ptr_);
  }

  bool operator!() const 
  {
    return !ptr_;
  }

  operator bool() const 
  {
    return !!ptr_;
  }

  uint64_t size() const 
  {
    return size_;
  }

private:
  uint64_t size_;
  std::shared_ptr<T> ptr_;  
};

class StringUtil
{
public:
  static std::string Format(const char* fmt, ...);
  static std::string Formatv(const char* fmt, va_list ap);

  static std::string UrlEncode(const std::string& str, bool space_as_plus = true);
  static std::string UrlDecode(const std::string& str, bool space_as_plus = true);

  static std::string Trim(const std::string& str, const std::string& delimit = " \t\r\n");
  static std::string TrimLeft(const std::string& str, const std::string& delimit = " \t\r\n");
  static std::string TrimRight(const std::string& str, const std::string& delimit = " \t\r\n");

  static std::string WStringToString(const std::wstring& ws);
  static std::wstring StringToWString(const std::string& s); 
};

std::string GetHostName();
std::string GetIPv4();

bool YamlToJson(const YAML::Node& ynode, Json::Value& jnode);
bool JsonToYaml(const Json::Value& jnode, YAML::Node& ynode);

template<typename T>
const char* TypeToName()
{
  static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
  return s_name;
}

std::string PBToJsonString(const google::protobuf::Message& message);

template<typename Iter>
std::string Join(Iter begin, Iter end, const std::string& tag)
{
  std::stringstream ss;
  for (Iter it = begin; it != end; it++)
  {
    if (it != begin)
    {
      ss << tag;
    }
    ss << *it;
  }

  return ss.str();
}

// ???
template<typename T>
int BinarySearch(const T* arr, int length, const T& v)
{
  int mid = 0;
  int left = 0;
  int right = length - 1;

  while(left <= right)
  {
    mid = (left + right) / 2;
    if (arr[mid] > v) right = mid - 1;
    else if (arr[mid < v]) left = mid + 1;
    else return mid;
  }

  return -(left + 1);
}

inline bool ReadFixFromStream(std::istream& is, char* data, const uint64_t& size)
{
  uint64_t pos = 0;
  while(is && (pos < size))
  {
    is.read(data + pos, size - pos); // 不一定就能一次读完，应该是为了避免读取失败的情况
    pos += is.gcount();
  }

  return pos == size;
}

template<typename T>
bool ReadFromStream(std::istream& is, T& v)
{
  return ReadFixFromStream(is, (char*)&v, sizeof(v));
}

template<class T>
bool ReadFromStream(std::istream& is, std::vector<T>& v) {
  return ReadFixFromStream(is, (char*)&v[0], sizeof(T) * v.size());
}

template<typename T>
bool WriteToStream(std::ostream& os, const T& v)
{
  if (!os) return false;
  os.write((const char*)&v, sizeof(T));
  return (bool)os;
}

template<class T>
bool WriteToStream(std::ostream& os, const std::vector<T>& v) 
{
  if(!os) {
      return false;
  }
  os.write((const char*)&v[0], sizeof(T) * v.size());
  return (bool)os;
}


/**
 * @brief 限速模块 
 */
class SpeedLimit
{
public:
  typedef std::shared_ptr<SpeedLimit> ptr;
  SpeedLimit(uint32_t speed);
  void add(uint32_t v);

private:
  uint32_t speed_;
  float countPerMS_;

  uint32_t curCount_;
  uint32_t curSec_;
};

bool ReadFixFromStreamWithSpeed(std::istream& is, char* data,
                                const uint64_t& size, const uint64_t& speed = -1);

bool WriteFixToStreamWithSpeed(std::ostream& os, const char* data,
                                const uint64_t& size, const uint64_t& speed = -1);
template<class T>
bool WriteToStreamWithSpeed(std::ostream& os, const T& v, const uint64_t& speed = -1) 
{
  if(os) 
  {
    return WriteFixToStreamWithSpeed(os, (const char*)&v, sizeof(T), speed);
  }
  return false;
}

template<class T>
bool WriteToStreamWithSpeed(std::ostream& os, const std::vector<T>& v, const uint64_t& speed = -1,
                            const uint64_t& min_duration_ms = 10) 
{
  if(os) 
  {
    return WriteFixToStreamWithSpeed(os, (const char*)&v[0], sizeof(T) * v.size(), speed);
  }
  return false;
}

template<class T>
bool ReadFromStreamWithSpeed(std::istream& is, const std::vector<T>& v, const uint64_t& speed = -1) 
{
  if(is) 
  {
    return ReadFixFromStreamWithSpeed(is, (char*)&v[0], sizeof(T) * v.size(), speed);
  }
  return false;
}

template<class T>
bool ReadFromStreamWithSpeed(std::istream& is, const T& v, const uint64_t& speed = -1) {
  if(is) 
  {
    return ReadFixFromStreamWithSpeed(is, (char*)&v, sizeof(T), speed);
  }
  return false;
}

std::string Format(const char* fmt, ...);
std::string Formatv(const char* fmt, va_list ap);

/**
 * @brief 按照size大小进行切片 
 */
template<typename T>
void Slice(std::vector<std::vector<T>>& dst, const std::vector<T>& src, size_t size)
{
  size_t left = src.size();
  size_t pos = 0;

  while (left > size)
  {
    std::vector<T> tmp;
    tmp.reserve(size); // 预留空间
    for (size_t i = 0; i < size; i++)
    {
      tmp.push_back(src[pos + i]);
    }
    pos += size;
    left -= size;
    dst.push_back(tmp);
  }

  if (left > 0)
  {
    std::vector<T> tmp;
    tmp.reserve(left);
    for (size_t i = 0; i < left; i++)
    {
      tmp.push_back(src[pos + i]);
    }
    dst.pop_back(tmp);
  }
}

}


#endif
