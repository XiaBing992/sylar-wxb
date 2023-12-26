#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <bits/types/time_t.h>
#include <cctype>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <functional>
#include <memory>
#include <ostream>
#include <sstream>
#include <iostream>
#include <tuple>
#include <vector>

#include "log.h"
#include "yaml-cpp/node/parse.h"
#include "yaml-cpp/yaml.h"

namespace sylar{
const char* LogLevel::ToString(LogLevel::Level level)
{
  switch(level)
  {
#define XX(name) \
    case LogLevel::Level::name: \
      return #name; \
      break;
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
      return "UNKNOW";
  }
  return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string &str)
{
#define XX(level, v) \
  if (str == #v) return LogLevel::Level::level;

  XX(DEBUG, DEBUG);
  XX(INFO, INFO);
  XX(WARN, WARN);
  XX(ERROR, ERROR);
  XX(FATAL, FATAL);

  XX(DEBUG, debug);
  XX(INFO, info);
  XX(WARN, warn);
  XX(ERROR, error);
  XX(FATAL, fatal);
#undef XX

  return LogLevel::Level::UNKNOW;
}

LogEventWrap::LogEventWrap(LogEvent::ptr e)
  : event_(e) {}

LogEventWrap::~LogEventWrap()
{
  event_->getLogger()->log(event_->getLevel(), event_);
}

void LogEvent::format(const char* fmt, ...)
{
  va_list al;
  va_start(al, fmt);
  format(fmt, al);
  va_end(al);
}

void LogEvent::format(const char* fmt, va_list al)
{
  char* buf = nullptr;
  int len = vasprintf(&buf, fmt, al);

  if  (len != -1)
  {
    ss_ << std::string(buf, len);
    free(buf);
  }
}

std::stringstream& LogEventWrap::getSS()
{
  return event_->getSS();
}

void LogAppender::setFormatter(LogFormatter::ptr val)
{
  // MutexTypeLock lock(mutex_);
  formatter_ = val;
  if (formatter_)
  {
    has_formatter_ = true;
  }
  else
  {
    has_formatter_ = false;
  }
}

LogFormatter::ptr LogAppender::getFormatter()
{
  // MutexType::Lock lock(mutex_);
  return formatter_;
}

class MessageFormatItem : public LogFormatter::FormatItem
{
public:
  MessageFormatItem(const std::string& str = ""){}

  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
  {
    os << event->getContent();
  }
};

class LevelFormatItem : public LogFormatter::FormatItem
{
public:
  LevelFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
  {
    os << LogLevel::ToString(level);
  }
};

class ElapseFormatItem : public LogFormatter::FormatItem
{
public:
  ElapseFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
  {
    os << event->getElapse();
  }
};

class NameFormatItem : public LogFormatter::FormatItem
{
public:
  NameFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
  {
    os << event->getLogger()->getName();
  }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem
{
public:
  ThreadIdFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
  {
    os << event->getThreadId();
  }
};

class FiberIdFormatItem : public LogFormatter::FormatItem
{
public:
  FiberIdFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
  {
    os << event->getFiberId();
  }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem
{
public:
  ThreadNameFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
  {
    os << event->getThreadName();
  }
};

class DateTimeFormatItem : public LogFormatter::FormatItem
{
public:
  DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
    : format_(format)
  {
    if (format_.empty())
    {
      format_ = "%Y-%m-%d %H:%M:%S";
    }
  }
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
  {
    struct tm tm;
    time_t time = event->getTime();
    localtime_r(&time, &tm); // 将时间转化成本地时间
    char buf[64];
    strftime(buf, sizeof(buf), format_.c_str(), &tm); // 将tm转成字符串格式
    os << buf;
  }
private:
  std::string format_;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
  FilenameFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
      os << event->getFile();
  }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
  LineFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
      os << event->getLine();
  }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
  NewLineFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
      os << std::endl;
  }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
  StringFormatItem(const std::string& str)
      :string_(str) {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
      os << string_;
  }
private:
  std::string string_;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
  TabFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
      os << "\t";
  }
private:
  std::string string_;
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
            ,const char* file, int32_t line, uint32_t elapse
            ,uint32_t thread_id, uint32_t fiber_id, uint64_t time
            ,const std::string& thread_name)
  :file_(file)
  ,line_(line)
  ,elapse_(elapse)
  ,thread_id_(thread_id)
  ,fiber_id_(fiber_id)
  ,time_(time)
  ,thread_name_(thread_name)
  ,logger_(logger)
  ,level_(level) {
}

Logger::Logger(const std::string& name)
  : name_(name)
  , level_(LogLevel::DEBUG) {
    formatter_.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::setFormatter(LogFormatter::ptr val)
{
  // MutexType::Lock lock(mutex_);
  formatter_ = val;

  for (auto& i : appenders_)
  {
    // MutexType::Lock ll(i>mutex_);
    if (!i->has_formatter_)
    {
      i->formatter_ = formatter_;
    }
  }
}

void Logger::setFormatter(const std::string& val)
{
  std::cout << "---" << val << std::endl;
  sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
  if (new_val->isError())
  {
    std::cout << "Logger setFormatter name=" << name_
              << "value=" << val << "invalid formatter"
              << std::endl;
    return;
  }

  setFormatter(new_val);
}

std::string Logger::toYamlString()
{
  // MutexType::Lock lock(mutex_);
  YAML::Node node;
  node["name"] = name_;
  if (level_ != LogLevel::UNKNOW)
  {
    node["level"] = LogLevel::ToString(level_);
  }
  if (formatter_)
  {
    node["formatter"] = formatter_->getPattern();
  }

  for (auto& i : appenders_)
  {
    node["appenders"].push_back(YAML::Load(i->toYamlString()));
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

LogFormatter::ptr Logger::getFormatter()
{
  // MutexType::Lock lock(mutex_);
  return formatter_;
}

void Logger::addAppender(LogAppender::ptr appender)
{
  // MutexType::Lock lock(mutex_);
  if (!appender->getFormatter())
  {
    // MutexType::Lock ll(appender->mutex_);
    appender->formatter_ = formatter_;
  }
  appenders_.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender)
{
  // MutexType::Lock lock(mutex_);
  for (auto it = appenders_.begin(); it != appenders_.end(); it++)
  {
    if (*it == appender)
    {
      appenders_.erase(it);
      break;
    }
  }
}

void Logger::clearAppenders()
{
  // MutexType::Lock lock(mutex_);
  appenders_.clear();
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event)
{
  if (level >= level_)
  {
    auto self = shared_from_this();
    // MutexType::Lock lock(mutex_);
    if (!appenders_.empty())
    {
      for (auto& i : appenders_)
      {
        i->log(self, level, event);
      }
    }
    else if (root_)
    {
      root_->log(level, event);
    }
  }
}

void Logger::debug(LogEvent::ptr event)
{
  log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event) {
    log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event) {
    log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event) {
    log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event) {
    log(LogLevel::FATAL, event);
}

FileLogAppender::FileLogAppender(const std::string& filename)
  : filename_(filename)
{
  //reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
  if (level >= level_)
  {
    uint64_t now = event->getTime();
    if (now >= (last_time_ + 3))
    {
      //reopen();
      last_time_ = now;
    }
    // MutexType::Lock lock(mutex_);
    if (!formatter_->format(filestream_, logger, level, event))
    {
      std::cout << "error" << std::endl;
    }
  }
}

std::string FileLogAppender::toYamlString()
{
  // MutexType::Lock lock(mutex_);
  YAML::Node node;
  node["type"] = "FileLogAppender";
  node["file"] = filename_;
  if (level_ != LogLevel::UNKNOW)
  {
    node["level"] = LogLevel::ToString(level_);
  }
  if (has_formatter_ && formatter_)
  {
    node["formatter"] = formatter_->getPattern();
  }

  std::stringstream ss;
  ss << node;
  return ss.str();
}

// bool FileLogAppender::reopen()
// {
//   // MutexType::Lock lock(mutex_);
//   if (filestream_)
//   {
//     filestream_.close();
//   }
//   return FSUtil::OpenForWrite(m_filestream, m_filename, std::ios::app);
// }

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) 
{
  if(level >= level_) {
      // MutexType::Lock lock(mutex_);
      formatter_->format(std::cout, logger, level, event);
  }
}

std::string StdoutLogAppender::toYamlString() {
  // MutexType::Lock lock(m_mutex);
  YAML::Node node;
  node["type"] = "StdoutLogAppender";
  if(level_ != LogLevel::UNKNOW) {
      node["level"] = LogLevel::ToString(level_);
  }
  if(has_formatter_ && formatter_) {
      node["formatter"] = formatter_->getPattern();
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

LogFormatter::LogFormatter(const std::string& pattern)
  : pattern_(pattern)
{
  init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
  std::stringstream ss;
  for (auto &i : items_)
  {
    i->format(ss, logger, level, event);
  }
  return ss.str();
}

std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
  for(auto& i : items_) 
  {
      i->format(ofs, logger, level, event);
  }
  return ofs;
}

void LogFormatter::init()
{
  //str, format, type
  std::vector<std::tuple<std::string, std::string, int>> vec;
  std::string nstr;

  for (size_t i = 0; i < pattern_.size(); i++)
  {
    if (pattern_[i] != '%')
    {
      nstr.append(1, pattern_[i]);
      continue;
    }

    // %% 特殊处理
    if ((i + 1) < pattern_.size())
    {
      if (pattern_[i + 1] == '%')
      {
        nstr.append(1, '%');
        continue;
      }
    }

    // 当前字符是%的处理
    size_t n = i + 1;
    int fmt_status = 0;
    size_t fmt_begin = 0; //标识解析格式

    std::string str;
    std::string fmt;
    while(n < pattern_.size())
    {
      if (!fmt_status && (!std::isalpha(pattern_[n]) && pattern_[n] != '{' 
            && pattern_[n] != '}'))
      {
        str = pattern_.substr(i + 1, n - i - 1);
        break;
      }

      if (fmt_status == 0)
      {
        if (pattern_[n] == '{')
        {
          str = pattern_.substr(i + 1);
          fmt_status = 1;
          fmt_begin = n;
          n++;
          continue;
        }
      }
      else
      {
        if (pattern_[n] == '}')
        {
          fmt = pattern_.substr(fmt_begin + 1, n - fmt_begin - 1);
          fmt_status = 0;
          n++;
          break;
        }
      }
      n++;
      if (n == pattern_.size())
      {
        if (str.empty())
        {
          str = pattern_.substr(i + 1);
        }
      }
    }

    if (fmt_status == 0)
    {
      if (!nstr.empty())
      {
        vec.push_back(std::make_tuple(nstr, std::string(), 0));
        nstr.clear();
      }
      vec.push_back(std::make_tuple(str, fmt, 1));
      i = n - 1;
    }
    else
    {
      std::cout << "pattern parse error: " << pattern_ << " - " << pattern_.substr(i) << std::endl;
      error_ = true;
      vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
    }
  }

  if (!nstr.empty())
  {
    vec.push_back(std::make_tuple(nstr, "", 0));
  }
  static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> format_items = {
#define XX(str, C) \
  {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));}}

  XX(m, MessageFormatItem),           //m:消息
  XX(p, LevelFormatItem),             //p:日志级别
  XX(r, ElapseFormatItem),            //r:累计毫秒数
  XX(c, NameFormatItem),              //c:日志名称
  XX(t, ThreadIdFormatItem),          //t:线程id
  XX(n, NewLineFormatItem),           //n:换行
  XX(d, DateTimeFormatItem),          //d:时间
  XX(f, FilenameFormatItem),          //f:文件名
  XX(l, LineFormatItem),              //l:行号
  XX(T, TabFormatItem),               //T:Tab
  XX(F, FiberIdFormatItem),           //F:协程id
  XX(N, ThreadNameFormatItem),        //N:线程名称
#undef XX
  };

  for (auto& i : vec)
  {
    if (std::get<2>(i) == 0)
    {
      items_.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
    }
    else
    {
      auto it = format_items.find(std::get<0>(i));
      if (it == format_items.end())
      {
        items_.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
        error_ = true;
      }
      else
      {
        items_.push_back(it->second(std::get<1>(i)));
      }
    }
  }
}

} // namespace sylar
