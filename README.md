# sylar-wxb
- 高性能服务器框架
- 感谢来源：https://github.com/sylar-yin/sylar

# 日志模块
![img](assets.assets/logger_uml.png)
## Logger 日志器
- 提供日志写入方法

## LogAppender 日志输出器
- 日志的输出方式

## LogFormat 日志格式器
- 日志的输出格式，参考log4j

## LogManager 日志管理器
- 管理所有日志器，通过YAML配置

## LogEvent 日志事件
- 将要写的日志

# 协程调度模块
- 线程池去运行调度器的run函数（执行调度器的逻辑）
- 线程池上跑多个协程，每个协程可以指定由哪个线程执行
- 空闲时运行idle函数，随时准备切换上下文。相应模块自己设定合适的idle函数

## 协程
- 每个协程5个状态：INIT(初始化) HOLD(swapin) EXEC(swapin) TERM（执行完毕） EXCEPT(异常)

## API
```
getcontext：保护现场，保存相关寄存器信息、栈信息
makecontext: 执行栈准备工作
swapcontext：执行上下文切换
```

# 定时器
- 定时回调函数的执行时间或者执行周期

## tips
- 会有一个函数检查存储时间的变量是否溢出

# IO协程调度器
- 继承了Scheduler，Timemanager
- 重写了idle函数，用epoll_wait等待事件响应
- 事件响应后，处理相应读写事件，之后swapout
- 支持设置定时任务，定时任务的触发事件会影响epoll_wait的超时时间
- 支持管道io和socket io

## tips
- epoll_data_t中的ptr可以与自定义的句柄关联起来，而fd只能用于关联文件描述符

# socket模块

# Problem
- 协程的底层原理
- log配置是怎么通过yaml传过去的
- 为什么要用extern c
- dlsym
- 成员函数指针