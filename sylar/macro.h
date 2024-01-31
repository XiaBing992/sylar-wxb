/*
 * @Author: Xiabing
 * @Date: 2024-01-29 21:13:00
 * @LastEditors: Xiabing
 * @LastEditTime: 2024-01-31 21:26:22
 * @FilePath: /sylar-wxb/sylar/macro.h
 * @Description: 
 */
#ifndef MACRO_H
#define MACRO_H

#include <assert.h>

#include "log.h"


// 编译器优化
#if defined __GNUC__ || defined __llvm__

#define SYLAR_LIKELY(x) __builtin_expect(!!(x), 1)

#define SYLAR_UNLIKELY(x) __builtin_expect(!!(x), 0)

#else

#define SYLAR_LIKELY(x) (x)

#define SYLAR_UNLIKELY(x) (x)


#endif

// 断言宏封装
#define SYLAR_ASSERT(x) \
  if (SYLAR_UNLIKELY(!(x))) \
  { \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    "); \
    assert(x); \
  }

#define SYLAR_ASSERT2(x, w) \
  if (SYLAR_UNLIKELY(!(x))) \
  { \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    "); \
    assert(x); \
  }

#endif