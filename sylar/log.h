/*
 * @Author: XiaBing
 * @Date: 2023-12-22 19:24:56
 * @LastEditors: XiaBing
 * @LastEditTime: 2024-01-03 16:55:03
 * @FilePath: /sylar-wxb/sylar/log.h
 * @Description: 
 */
#ifndef LOG_H
#define LOG_H

#include <bits/stdint-intn.h>
#include <bits/types/FILE.h>
#include <cstdarg>
#include <fstream>
#include <ostream>
#include <string>
#include <sstream>
#include <memory>
#include <sys/types.h>
#include <vector>
#include <list>
#include <map>

#include "singleton.h"
#include "thread.h"
#include "util.h"
#include "mutex.h"

/**
 * @brief 使用流式方式将日志写入logger 
 */
#define SYLAR_LOG_LEVEL(logger, level) \
  if (logger->getLevel() <= level) \
    sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, sylar::GetThreadId(), sylar::GetFiberId(), \
                        time(0), sylar::Thread::GetName()))).getSS()

/**
 * @brief 使用流式方式将日志级别debug的日志写入到logger
 */
#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)

/**
 * @brief 使用流式方式将日志级别info的日志写入到logger
 */
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)

/**
 * @brief 使用流式方式将日志级别warn的日志写入到logger
 */
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)

/**
 * @brief 使用流式方式将日志级别error的日志写入到logger
 */
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)

/**
 * @brief 使用流式方式将日志级别fatal的日志写入到logger
 */
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

/**
 * @brief 使用格式化方式将日志写入logger 
 */
#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...) \
  if (logger->getLevel() <= level) \
    sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
      __FILE__, __LINE__, 0, sylar::getThreadId(), \
        sylar::getFiberId(), time(0), sylar::Thread::GetName()))).getEvent->format(fmt, __VA_ATGS__)

/**
 * @brief 使用格式化方式将日志级别debug的日志写入到logger
 */
#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别info的日志写入到logger
 */
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别warn的日志写入到logger
 */
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别error的日志写入到logger
 */
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别fatal的日志写入到logger
 */
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)

/**
 * @brief 获取主日志器 
 */
#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()

/**
 * @brief 获取name的日志器 
 */
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)


namespace sylar{

class Logger;
class LoggerManager;

/**
 * @brief 日志级别枚举
 */

class LogLevel
{
public:
  /**
  * @brief 日志级别
  */
  enum Level
  {
    UNKNOW = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
  };

  /**
  * @brief 日志级别转文本输出
  * @param level 日志级别
  */
  static const char* ToString(LogLevel::Level level);

  /**
  * @brief 将文本转换成日志级别
  * @param str 日志级别文本
  */
  static LogLevel::Level FromString(const std::string &str);
};

/**
 * @brief 日志事件
 */
class LogEvent
{
public:
  typedef std::shared_ptr<LogEvent> ptr;
 
  LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
         , const char* file, int32_t line, u_int32_t elapse
         , u_int32_t thread_id, u_int32_t fiber_id, u_int64_t time
         , const std::string& thread_name);
  
  const char* getFile() const { return file_;}

  int32_t getLine() const { return line_;}
  
  u_int32_t getElapse() const { return elapse_;}

  u_int32_t getThreadId() const { return thread_id_;}

  u_int32_t getFiberId() const { return fiber_id_;}

  u_int64_t getTime() const { return time_;}

  const std::string& getThreadName() { return thread_name_;}

  std::string getContent() const { return ss_.str();}

  std::shared_ptr<Logger> getLogger() const { return logger_;}

  LogLevel::Level getLevel() const { return level_;}

  std::stringstream& getSS() { return ss_;}

  /**
  * @brief 格式化写入日志内容
  */
  void format(const char* fmt, ...);

  /**
  * @brief 格式化写入日志内容
  */
  void format(const char* fmt, va_list al);
private:
  const char* file_ = nullptr; // 文件名
  
  int32_t line_ = 0; // 行号
  
  u_int32_t elapse_ = 0; //程序启动到现在的时间
  
  u_int32_t thread_id_ = 0; //线程id
  
  u_int32_t fiber_id_ = 0; // 协程id
  
  u_int64_t time_ = 0; // 时间戳
  
  std::string thread_name_; // 线程名称
  
  std::stringstream ss_; //日志内容流
  
  std::shared_ptr<Logger> logger_; // 日志器
  
  LogLevel::Level level_; // 日志等级
};

/**
 * @brief 日志事件包装器
 */
class LogEventWrap
{
public:
  LogEventWrap(LogEvent::ptr e);

  ~LogEventWrap();

  LogEvent::ptr getEvent() const { return event_;}

  /**
   * @brief 获取日志内容流 
   */  
  std::stringstream& getSS();
private:
  LogEvent::ptr event_; // 日志事件
};

/**
 @brief 日志格式化
 */

class LogFormatter
{
public:
  typedef std::shared_ptr<LogFormatter> ptr;
  
  /**
   * @brief: 
   * @param {string&} pattern 格式模板
   * @details
   *  %m 消息
   *  %p 日志级别
   *  %r 累计毫秒数
   *  %p 日志名称
   *  %c 日志级别
   *  %t 线程id
   *  %n 换行
   *  %d 时间
   *  %f 文件名
   *  %l 行号
   *  %T 制表符
   *  %F 协程if
   *  %N 线程名称
   *
   *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
   */  
  LogFormatter(const std::string& pattern);

  /**
   * @brief 格式化日志文件
   * @param logger 日志器
   * @param level 日志级别
   * @param event 日志事件
   */  
  std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
  std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

public:
  /**
   * 日志内容格式化
   */
  class FormatItem
  {
  public:
    typedef std::shared_ptr<FormatItem> ptr;

    virtual ~FormatItem() {}

    virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

  };

  /**
   * @brief 初始化，解析日志模板 
   */  
  void init();

  /**
   * @brief 是否有错误 
   */  
  bool isError() const { return error_;}

  /**
   * @brief 返回日志模板 
   */  
  const std::string getPattern() const { return pattern_;}


private:
  std::string pattern_; //日志格式模板
  std::vector<FormatItem::ptr> items_; // 日志格式解析后的格式
  bool error_ = false; // 是否有错误

};

/**
 * @brief 日志输出模板
 */
class LogAppender
{
friend class Logger;
public:
  typedef std::shared_ptr<LogAppender> ptr;
  typedef Spinlock MutexType;

  virtual ~LogAppender() {}

  /**
   * @brief 写日志
   * @param logger 日志器
   * @param level 日志级别
   * @param event 日志事件
   */  
  virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

  /**
   * @brief 将日志输出目标配置转成YAML String
   */  
  virtual std::string toYamlString() = 0;
 
  void setFormatter(LogFormatter::ptr val);
  
  LogFormatter::ptr getFormatter();

  LogLevel::Level getLevel() const { return level_;}

  void setLevel(LogLevel::Level val) { level_ = val;}

protected:
  LogLevel::Level level_ = LogLevel::Level::DEBUG; //日志级别

  bool has_formatter_ = false; // 是否有自己的日志格式器

  MutexType mutex_;

  LogFormatter::ptr formatter_; // 日志格式器
};

/**
 * @brief 日志器
 */
class Logger : public std::enable_shared_from_this<Logger>
{
friend class LoggerManager;
public:
  typedef std::shared_ptr<Logger> ptr;
  typedef Spinlock MutexType;

  Logger(const std::string& name = "root");

  /**
   * @brief 写日志
   * @param level 日志级别
   * @param event 日志事件
   */  
  void log(LogLevel::Level level, LogEvent::ptr event);

  void debug(LogEvent::ptr event);

  void info(LogEvent::ptr event);

  void warn(LogEvent::ptr event);

  void error(LogEvent::ptr event);

  void fatal(LogEvent::ptr event);

  /**
   * @brief 添加日志目标 
   * @param appender 日志目标
   */  
  void addAppender(LogAppender::ptr appender);

  void delAppender(LogAppender::ptr appender);

  void clearAppenders();

  LogLevel::Level getLevel() const { return level_;}

  void setLevel(LogLevel::Level level) { level_ = level;}

  const std::string& getName() const { return name_;}

  /**
   * @brief 设置日志格式器 
   */  
  void setFormatter(LogFormatter::ptr val);

  /**
   * @brief 设置日志格式模板
   */  
  void setFormatter(const std::string& val);

  LogFormatter::ptr getFormatter();

  /**
   * @brief 将日志器的配置转成YAML String 
   */  
  std::string toYamlString();


private:
  std::string name_; // 日志名称

  LogLevel::Level level_;

  MutexType mutex_;

  std::list<LogAppender::ptr> appenders_; // 日志目标集合

  LogFormatter::ptr formatter_; // 日志格式器

  Logger::ptr root_; // 主日志器
};

/**
 * @brief 输出到控制台的Appender
 */
class StdoutLogAppender : public LogAppender
{
public:
  typedef std::shared_ptr<StdoutLogAppender> ptr;

  void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

  std::string toYamlString() override;
};

/**
 * @brief 输出到文件的Appender
 */
class FileLogAppender : public LogAppender
{
public:
  typedef std::shared_ptr<FileLogAppender> ptr;
  
  FileLogAppender(const std::string& filename);

  void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

  std::string toYamlString() override;

  /**
   * @brief 重新弄打开日志文件 
   */  
  // bool reopen();

private:
  std::string filename_;

  std::ofstream filestream_;

  u_int64_t last_time_ = 0; // 上次重新打开时间
};

/**
 * @brief 日志器管理类
 */
class LoggerManager
{
public:
  typedef Spinlock MutexType;

  LoggerManager();

  Logger::ptr getLogger(const std::string& name);

  void init();

  Logger::ptr getRoot() const { return root_;}

  std::string toYamlString();

private:
  MutexType mutex_;

  std::map<std::string, Logger::ptr> loggers_;

  Logger::ptr root_; // 主日志器
};

typedef sylar::Singleton<LoggerManager> LoggerMgr;


} // namespace sylar

#endif