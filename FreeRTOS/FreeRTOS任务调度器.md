---
notion-id: 37f5d378-8ef5-80e6-982e-fc8026fc1f79
---
# 开启任务调度器
vTaskStartScheduler() 

作用：用于启动任务调度器，任务调度器启动后， FreeRTOS 便会开始进行任务调度

该函数内部实现，如下:

1. 创建空闲任务
2. 如果使能软件定时器,则创建定时器任务
3. 关闭中断,防止调度器开启之前或过程中,受中断感染,会在运行第一个任务时期打开中断
4. 初始化全局变量,并将任务调度器的运行标志设置为已运行
5. 初始化任务运行时间统计功能的时基定时器
6. 调用函数 xPortStartScheduler()

#  xPortStartScheduler()

作用：该函数用于完成启动任务调度器中与硬件架构相关的配置部分，以及启动第一个任务

该函数内部实现，如下：

7. 检测用户在 FreeRTOSConfig.h 文件中对中断的相关配置是否有误
8. 配置 PendSV 和 SysTick 的中断优先级为最低优先级
9. 调用函数 vPortSetupTimerInterrupt()配置 SysTick
10. 初始化临界区嵌套计数器为 0
11. 调用函数 prvEnableVFP()使能 FPU
12. 调用函数 prvStartFirstTask()启动第一个任务