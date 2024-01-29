/*
 * @Author: XiaBing
 * @Date: 2023-12-22 19:25:36
 * @LastEditors: XiaBing
 * @LastEditTime: 2024-01-24 09:40:38
 * @FilePath: /sylar-wxb/tests/test.cc
 * @Description: 
 */
#include <iostream>
#include "log.h"
#include "util.h"

int main()
{
  // 自定义logger使用方式
  sylar::Logger::ptr logger(new sylar::Logger);

  // 控制台输出日志
  logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));

  // 文件输出日志
  sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("./log.txt"));
  sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d%T%p%T%m%n"));
  file_appender->setFormatter(fmt);
  file_appender->setLevel(sylar::LogLevel::ERROR);

  logger->addAppender(file_appender);

  SYLAR_LOG_INFO(logger) << "test macro";
  SYLAR_LOG_ERROR(logger) << "test macro error";

  SYLAR_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");

  // 通过单例使用
  auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
  SYLAR_LOG_INFO(l) << "xxx";

  return 0;
}