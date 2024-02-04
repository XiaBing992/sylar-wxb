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

## 协程
- 每个协程5个状态：INIT(初始化) HOLD(swapin) EXEC(swapin) TERM（执行完毕） EXCEPT(异常)

## API
```
getcontext：保护现场，保存相关寄存器信息、栈信息
makecontext: 执行栈准备工作
swapcontext：执行上下文切换
```