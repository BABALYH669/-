---
notion-id: 3775d378-8ef5-80ef-9385-d88983edc875
---
## 相关宏大致可分为三类

1. INCLUDE  配置FreeRTOS中可选的API函数
2. config  完成FreeRTOS的功能配置和裁剪
3. 其他配置项,PendSV宏定义,SVC宏定义

```javascript
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*===========================================================================
 * 头文件包含
 *===========================================================================*/
#include "./SYSTEM/sys/sys.h"           // 系统支持（时钟、初始化等）
#include "./SYSTEM/usart/usart.h"       // 串口支持
#include <stdint.h>                     // 标准整型定义

extern uint32_t SystemCoreClock;        // 系统主频（由 system_stm32f10x.c 定义，通常 72MHz）

/*===========================================================================
 * 一、基础调度配置
 *===========================================================================*/
#define configUSE_PREEMPTION                            1
/* 1=抢占式调度器：高优先级任务就绪时立即打断低优先级任务
   0=协程式调度器：任务必须主动让出 CPU（调用 taskYIELD 或阻塞）                         */

#define configUSE_PORT_OPTIMISED_TASK_SELECTION         1
/* 1=使用硬件"前导零计数指令 CLZ"快速选出最高优先级就绪任务（快,适合 32 级以下）
   0=使用软件循环查表算法（适合优先级数超过 32 的场景）                                 */

#define configUSE_TICKLESS_IDLE                         0
/* 1=启用 tickless 低功耗模式：空闲时停止 SysTick，等有任务唤醒时再恢复
   0=关闭（常规模式：SysTick 一直跑）                                                */

#define configCPU_CLOCK_HZ                              SystemCoreClock
/* CPU 主频，单位 Hz。SystemCoreClock 通常是 72000000（72MHz）                      */

#define configTICK_RATE_HZ                              1000
/* 系统时钟节拍频率（SysTick 中断频率），1000Hz = 每 1ms 产生一次 tick 中断            */

#define configMAX_PRIORITIES                            32
/* 最大优先级数量，取值范围 0 ~ 31，数字越大优先级越高。0 是最低优先级（仅空闲任务用）      */

#define configMINIMAL_STACK_SIZE                        128
/* 空闲任务栈大小，单位是 word（1 word = 4 bytes），128×4 = 512 字节                   */

#define configMAX_TASK_NAME_LEN                         16
/* 任务名的最大字符数（包括结尾 '\0'），即最多 15 个有效字符                              */

#define configUSE_16_BIT_TICKS                          0
/* 0=tick 计数器用 32 位（约 49 天才溢出，推荐）
   1=tick 计数器用 16 位（省内存，但约 65 秒就溢出，只能配合 configTICK_RATE_HZ≤1000 用） */

#define configIDLE_SHOULD_YIELD                         1
/* 1=抢占式调度下，同优先级的用户任务可以抢占空闲任务（推荐开启）
   0=空闲任务不会被同优先级任务抢占                                                  */

#define configUSE_TASK_NOTIFICATIONS                    1
/* 1=启用任务通知功能（比信号量/队列快 45%、省 RAM，是 FreeRTOS 最轻量的任务间通信方式）   */

#define configTASK_NOTIFICATION_ARRAY_ENTRIES           1
/* 每个任务的通知数组大小，默认 1（即每个任务只有一个通知值）                             */

#define configUSE_MUTEXES                               1
/* 1=启用互斥信号量（Mutex）：带优先级继承机制，防止优先级反转问题                         */

#define configUSE_RECURSIVE_MUTEXES                     1
/* 1=启用递归互斥信号量：同一个任务可以多次获取该锁而不会死锁（获取几次就要释放几次）         */

#define configUSE_COUNTING_SEMAPHORES                   1
/* 1=启用计数信号量：可以管理多个资源实例（比如管理 5 个串口缓冲区）                        */

#define configUSE_ALTERNATIVE_API                       0
/* 【已废弃！！】以前老版本的另一套 API，新版不用，保持 0                                 */

#define configQUEUE_REGISTRY_SIZE                       8
/* 可注册到"队列注册表"的队列/信号量/互斥锁的最大个数
   注册后可以用 vQueueAddToRegistry() 调试时查看队列状态，0 表示禁用注册功能             */

#define configUSE_QUEUE_SETS                            1
/* 1=启用队列集：一个任务可以同时监听多个队列（类似 Linux 的 select/epoll 多路复用）       */

#define configUSE_TIME_SLICING                          1
/* 1=启用时间片轮转调度：同优先级的多个任务轮流执行，每个 tick 切换一次
   0=同优先级任务必须主动让出 CPU 或阻塞才会切换                                       */

#define configUSE_NEWLIB_REENTRANT                      0
/* 1=每个任务创建时分配 Newlib 重入结构体（使用标准 C 库如 printf/scanf 时需设为 1）
   0=不使用（如果用了 printf 会出问题，建议搭配 MicroLib 或用任务锁保护）                */

#define configENABLE_BACKWARD_COMPATIBILITY             0
/* 0=禁用老版本兼容 API（节省代码空间，推荐）
   1=启用（新老 API 都能用，但浪费 Flash）                                           */

#define configNUM_THREAD_LOCAL_STORAGE_POINTERS         0
/* 每个任务的线程本地存储指针数（类似 C11 的 _Thread_local），0 表示不需要                  */

#define configSTACK_DEPTH_TYPE                          uint16_t
/* 任务栈深度（即栈有多少个 word）的数据类型，uint16_t 最大 65535 word ≈ 256KB 栈         */

#define configMESSAGE_BUFFER_LENGTH_TYPE                size_t
/* 消息缓冲区中 消息长度字段 的数据类型（用于 Stream Buffer / Message Buffer）            */

/*===========================================================================
 * 二、内存分配相关
 *===========================================================================*/
#define configSUPPORT_STATIC_ALLOCATION                 0
/* 0=不支持静态分配（不能用 xTaskCreateStatic 在编译时指定栈和 TCB 的内存）
   1=支持静态分配（适合安全关键系统，避免动态内存的不确定性）                              */

#define configSUPPORT_DYNAMIC_ALLOCATION                1
/* 1=支持动态分配（可以用 xTaskCreate 从 FreeRTOS 堆中自动申请栈和 TCB 内存）
   0=不支持（必须用静态分配）                                                         */

#define configTOTAL_HEAP_SIZE                           ((size_t)(10 * 1024))
/* FreeRTOS 堆的总大小 = 10KB。所有任务栈、TCB、队列、信号量都从这里面分配                 */

#define configAPPLICATION_ALLOCATED_HEAP                0
/* 0=FreeRTOS 自动声明 ucHeap[] 数组作为堆空间
   1=用户手动在别处声明 uint8_t ucHeap[configTOTAL_HEAP_SIZE]                        */

#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP       0
/* 0=任务栈和内核对象（队列、信号量）都从同一个 FreeRTOS 堆分配
   1=任务栈由用户自定义的 pvPortMallocStack()/vPortFreeStack() 分配                   */

/*===========================================================================
 * 三、钩子函数（Hook）相关
 *===========================================================================*/
#define configUSE_IDLE_HOOK                             0
/* 1=启用空闲任务钩子：每轮空闲循环都会调用 vApplicationIdleHook()
   可用于进入低功耗模式、喂狗、统计 CPU 空闲率等。注意：钩子里不能阻塞！                    */

#define configUSE_TICK_HOOK                             0
/* 1=启用 tick 钩子：每个 tick 中断里都会调用 vApplicationTickHook()
   可用于做一个简单的微秒级定时任务，但建议尽量短小，不要在里面阻塞                       */

#define configCHECK_FOR_STACK_OVERFLOW                  0
/* 栈溢出检测：0=关闭  1=方法一（检测栈顶是否溢出）  2=方法二（水印法，推荐开发时用 2）
   检测到溢出会调用 vApplicationStackOverflowHook() 钩子                              */

#define configUSE_MALLOC_FAILED_HOOK                    0
/* 1=启用内存申请失败钩子：pvPortMalloc() 失败时调用 vApplicationMallocFailedHook()
   可用来记录错误日志或执行紧急处理                                                    */

#define configUSE_DAEMON_TASK_STARTUP_HOOK              0
/* 1=启用定时器服务任务（Daemon Task）首次执行前的钩子 vApplicationDaemonTaskStartupHook()
   注意：需要 configUSE_TIMERS=1 时才有意义                                           */

/*===========================================================================
 * 四、运行时统计 & 任务状态
 *===========================================================================*/
#define configGENERATE_RUN_TIME_STATS                   0
/* 1=启用任务运行时间统计（可以看每个任务占用了多少 CPU 时间）
   开启后需要额外提供一个比 tick 更精细的定时器，见下方条件编译                          */

#if configGENERATE_RUN_TIME_STATS
    #include "./BSP/TIMER/btim.h"      // 基本定时器驱动
    #define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()  ConfigureTimeForRunTimeStats()
    /* 初始化用于运行时间统计的定时器（比如 TIM6，10kHz = 0.1ms 分辨率）                  */
    extern uint32_t FreeRTOSRunTimeTicks;
    /* 运行时统计用的高精度 tick 计数器（在定时器中断里自增）                              */
    #define portGET_RUN_TIME_COUNTER_VALUE()           FreeRTOSRunTimeTicks
    /* 获取当前运行时计数值                                                             */
#endif

#define configUSE_TRACE_FACILITY                        1
/* 1=启用可视化跟踪/调试功能
   会编译 uxTaskGetSystemState()、vTaskList() 等函数，配合 IDE 插件调试                 */

#define configUSE_STATS_FORMATTING_FUNCTIONS            1
/* 1=编译 vTaskList() 和 vTaskGetRunTimeStats() 函数
   可打印出格式化的任务状态表格（任务名、状态、优先级、栈剩余、运行时间等）                */

/*===========================================================================
 * 五、协程（Co-Routine）相关 —— 基本废弃不用
 *===========================================================================*/
#define configUSE_CO_ROUTINES                           0
/* 1=启用协程功能（协程比任务更轻量，但已不推荐使用，用任务+任务通知替代）                  */

#define configMAX_CO_ROUTINE_PRIORITIES                 2
/* 协程最大优先级数（仅在 configUSE_CO_ROUTINES=1 时有意义）                             */

/*===========================================================================
 * 六、软件定时器相关
 *===========================================================================*/
#define configUSE_TIMERS                                1
/* 1=启用软件定时器（不占用硬件 TIM 资源，由定时器服务任务统一管理）                        */

#define configTIMER_TASK_PRIORITY                       ( configMAX_PRIORITIES - 1 )
/* 定时器服务任务的优先级 = 31（最高级），确保定时器回调能及时执行                         */

#define configTIMER_QUEUE_LENGTH                        5
/* 定时器命令队列长度：最多同时有 5 个待处理的定时器操作（启动/停止/复位等）                 */

#define configTIMER_TASK_STACK_DEPTH                    ( configMINIMAL_STACK_SIZE * 2 )
/* 定时器服务任务栈大小 = 128×2 = 256 word = 1024 字节                                 */

/*===========================================================================
 * 七、可选 API 函数开关（1=编译到固件，0=不编译以节省 Flash）
 *===========================================================================*/
#define INCLUDE_vTaskPrioritySet                        1
/* vTaskPrioritySet()     — 设置任务优先级                                            */

#define INCLUDE_uxTaskPriorityGet                       1
/* uxTaskPriorityGet()    — 获取任务优先级                                            */

#define INCLUDE_vTaskDelete                             1
/* vTaskDelete()          — 删除任务（释放 TCB 和栈）                                   */

#define INCLUDE_vTaskSuspend                            1
/* vTaskSuspend()         — 挂起任务（让任务暂停，不参与调度，需手动恢复）                 */

#define INCLUDE_xResumeFromISR                          1
/* xTaskResumeFromISR()   — 在中断中恢复被挂起的任务                                    */

#define INCLUDE_vTaskDelayUntil                         1
/* vTaskDelayUntil()      — 绝对延时（保证任务以固定周期执行，不受执行时间影响）           */

#define INCLUDE_vTaskDelay                              1
/* vTaskDelay()           — 相对延时（从调用时刻起延时 N 个 tick）                       */

#define INCLUDE_xTaskGetSchedulerState                  1
/* xTaskGetSchedulerState() — 获取调度器状态（运行中/挂起/未启动）                        */

#define INCLUDE_xTaskGetCurrentTaskHandle               1
/* xTaskGetCurrentTaskHandle() — 获取当前正在运行的任务句柄                              */

#define INCLUDE_uxTaskGetStackHighWaterMark             1
/* uxTaskGetStackHighWaterMark() — 获取任务栈历史最小剩余值
   这是调试栈大小的利器，返回值越小说明越接近栈溢出！                                    */

#define INCLUDE_xTaskGetIdleTaskHandle                  1
/* xTaskGetIdleTaskHandle() — 获取空闲任务句柄（极少直接使用）                            */

#define INCLUDE_eTaskGetState                           1
/* eTaskGetState()        — 获取任务当前状态（就绪/运行/阻塞/挂起/删除）                  */

#define INCLUDE_xEventGroupSetBitFromISR                1
/* xEventGroupSetBitFromISR() — 在中断中设置事件标志位                                  */

#define INCLUDE_xTimerPendFunctionCall                  1
/* xTimerPendFunctionCall() — 把一个函数挂到定时器服务任务中执行（类似 runOnMainThread）   */

#define INCLUDE_xTaskAbortDelay                         1
/* xTaskAbortDelay()      — 强制中断一个正在延时的任务，使其立即就绪                      */

#define INCLUDE_xTaskGetHandle                          1
/* xTaskGetHandle()       — 通过任务名字符串获取任务句柄                                 */

#define INCLUDE_xTaskResumeFromISR                      1
/* xTaskResumeFromISR()   — 在中断中恢复挂起的任务（与上面 xResumeFromISR 功能相同）      */

/*===========================================================================
 * 八、中断嵌套行为配置（核心配置！）
 *===========================================================================*/
#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS    __NVIC_PRIO_BITS   // 由 CMSIS 头文件提供
#else
    #define configPRIO_BITS    4                   // STM32F103 CM3 使用 4 位优先级
#endif
/* STM32F103 Cortex-M3：优先级寄存器 8 位，只用了高 4 位 → 0~15 共 16 个优先级           */

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
/* 库函数视角的最低中断优先级 = 15（数值最大 = 最不紧急）                                 */

#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
/* FreeRTOS 系统调用能安全使用的最高优先级 = 5
   → 优先级 5~15 的中断中可以调用 FreeRTOS API（如 xSemaphoreGiveFromISR）
   → 优先级 0~4  的中断中 不能 调用 FreeRTOS API（否则破坏内核数据结构）                 */

#define configKERNEL_INTERRUPT_PRIORITY                 ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
/* 内核异常（PendSV、SysTick）的优先级 = 15 << 4 = 0xF0（最低优先级，不干扰任何用户中断）  */

#define configMAX_SYSCALL_INTERRUPT_PRIORITY            ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
/* 系统调用可管理的最高优先级 = 5 << 4 = 0x50（优先级 5 对应 NVIC 寄存器值 0x50）         */

#define configMAX_API_CALL_INTERRUPT_PRIORITY           configMAX_SYSCALL_INTERRUPT_PRIORITY
/* 别名，含义同上。FreeRTOS API 只能在优先级 ≤ 这个值的中断中调用                         */

/*===========================================================================
 * 九、FreeRTOS 中断服务函数映射（连接到启动文件的异常向量表）
 *===========================================================================*/
#define xPortPendSVHandler      PendSV_Handler
/* PendSV（可挂起系统调用）用于执行上下文切换，设为最低优先级，让切换发生在所有中断之后      */

#define vPortSVCHandler         SVC_Handler
/* SVC（系统服务调用）用于启动第一个任务，在 vTaskStartScheduler() 中触发                  */

/*===========================================================================
 * 十、断言（Assert）调试
 *===========================================================================*/
#define vAssertCalled(char, int)    printf("Error: %s, %d\r\n", char, int)
/* 断言失败时的回调：打印错误文件路径和行号                                             */

#define configASSERT( x )           if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ )
/* 如果参数 x 为假（0），触发断言。调试时开启，发布时通常会关闭以节省代码空间              */

/*===========================================================================
 * 十一、MPU（内存保护单元）相关 —— STM32F103 不支持，全部注释
 *===========================================================================*/
//#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS  0
//#define configTOTAL_MPU_REGIONS                                 8
//#define configTEX_S_C_B_FLASH                                   0x07UL
//#define configTEX_S_C_B_SRAM                                    0x07UL
//#define configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY             1
//#define configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS              1

/*===========================================================================
 * 十二、ARMv8-M 安全侧端口 —— STM32F103 不支持 TrustZone，不需配置
 *===========================================================================*/
//#define secureconfigMAX_SECURE_CONTEXTS                         5

#endif /* FREERTOS_CONFIG_H */
```
