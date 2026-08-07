---
notion-id: 37a5d378-8ef5-80f0-ac8e-f9863d92af2a
---
# 什么是中断?

让CPU打断正常运行的程序,转而去处理紧急的事件(程序),就叫中断

中断请求:外设产生中断请求(GPIO外部中断,定时器中断)
相应中断:CPU停止执行当前程序,转而去指向中断处理程序(ISP)
推出中断:执行完毕,返回被打断的程序处,继续往下执行

# 中断优先级分组设置

ARM Cortex-M 使用了8位宽的寄存器来配置中断的优先等级,这个寄存器就是中断优先级寄存器

8位宽是2^8 = 256 中断优先级分组就是0~255

但是STM32,只用了中断优先级配置寄存器的高四位[7:4],所以STM32提供了最大16级的中断优先级 2^4 = 16

STM32的中断优先级分为抢占优先级和子优先级
**抢占优先级**:抢占优先级高的可以打断正在被执行但抢占优先级低的中断
**子优先级**:当同时发生具有相同抢占优先级的两个中断时，子优先级数值小的优先执行
中断优先级数值越小越优先

一共有 5 种分配方式，对应着中断优先级分组的 5 个组
在HALInit中设置:
HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4）

| 优先级分组 | 抢占优先级 | 子优先级 | 优先级配置寄存器高 4 位 |
| --- | --- | --- | --- |
| NVIC_PriorityGroup_0 | 0 级抢占优先级 | 0-15 级子优先级 | 0bit 用于抢占优先级4bit 用于子优先级 |
| NVIC_PriorityGroup_1 | 0-1 级抢占优先级 | 0-7 级子优先级 | 1bit 用于抢占优先级3bit 用于子优先级 |
| NVIC_PriorityGroup_2 | 0-3 级抢占优先级 | 0-3 级子优先级 | 2bit 用于抢占优先级2bit 用于子优先级 |
| NVIC_PriorityGroup_3 | 0-7 级抢占优先级 | 0-1 级子优先级 | 3bit 用于抢占优先级1bit 用于子优先级 |
| NVIC_PriorityGroup_4 | 0-15 级抢占优先级 | 0 级子优先级 | 4bit 用于抢占优先级0bit 用于子优先级 |

特点:

1. 低于configMAX_SYSCALL_INTERRUPT_PRIORITY(5)优先级的中断里才允许调用FreeRTOS 的API函数
2. 建议将所有优先级位指定为抢占优先级位，方便FreeRTOS管理
3. 中断优先级数值越小越优先，任务优先级数值越大越优先
![[wps_wmmvrnNxOh.png]]

# 中断相关寄存器

三个系统中断优先级配置寄存器，分别为 SHPR1、 SHPR2、 SHPR3 一个寄存器四个位,一个位8个字节
SHPR1寄存器地址：0xE000ED18
SHPR2寄存器地址：0xE000ED1C
SHPR3寄存器地址：0xE000ED20

![[wps_KQoaXQqmq8.png]]

实现的效果是0~15左移4个位,因为在STM32中低四位没有用

![[wps_xBhplgEwrs.png]]

三个中断屏蔽寄存器，分别为 PRIMASK、 FAULTMASK 和BASEPRI
BASEPRI：屏蔽优先级低于某一个阈值的中断
BASEPRI设置为0x50，代表中断优先级在5~15内的均被屏蔽，0~4的中断优先级正常执行
FreeRTOS所使用的中断管理就是利用的BASEPRI这个寄存器

![[wps_axvCHhV95i.png]]

![[wps_GtCyHUWewD.png]]

![[wps_MzRwQ02XKT.png]]

![[wps_bSaS11VeKa.png]]

在中断服务函数中调度FreeRTOS的API函数需注意
1. 中断服务函数的优先级需在FreeRTOS所管理的范围内
2.在中断服务函数里边需调用FreeRTOS的API函数，必须使用带“FromISR”后缀的函数


# FreeRTOS中断管理实验

| 开中断 | portENABLE_INTERRUPTS() |
| --- | --- |
| 关中断 | portDISABLE_INTERRUPTS()  |

# 一、`btim.h` — 定时器硬件宏定义

```c
#ifndef __BTIM_H
#define __BTIM_H

#include "./SYSTEM/sys/sys.h"

/* ========== TIM6 宏定义 ========== */
#define BTIM_TIM6_INT                       TIM6                // 外设基地址
#define BTIM_TIM6_INT_IRQn                  TIM6_IRQn           // NVIC 中断号
#define BTIM_TIM6_INT_IRQHandler            TIM6_IRQHandler     // 中断服务函数名
#define BTIM_TIM6_INT_CLK_ENABLE()          do{ __HAL_RCC_TIM6_CLK_ENABLE(); }while(0)  // 时钟使能

/* ========== TIM7 宏定义 ========== */
#define BTIM_TIM7_INT                       TIM7
#define BTIM_TIM7_INT_IRQn                  TIM7_IRQn
#define BTIM_TIM7_INT_IRQHandler            TIM7_IRQHandler
#define BTIM_TIM7_INT_CLK_ENABLE()          do{ __HAL_RCC_TIM7_CLK_ENABLE(); }while(0)

/* ========== 函数声明 ========== */
void btim_tim6_int_init(uint16_t arr, uint16_t psc);   // arr=重装载值, psc=预分频值
void btim_tim7_int_init(uint16_t arr, uint16_t psc);

#endif
```

**作用**：用宏把 TIM6 和 TIM7 的五项硬件资源统一封装，后续写代码时不直接写 `TIM6` 而是写 `BTIM_TIM6_INT`，方便移植。

---

# 二、`btim.c` — 定时器驱动（5 个函数）

## 2.1 全局句柄（第 6 ~ 7 行）

```c
TIM_HandleTypeDef g_tim6_handle;   // STM32 HAL 库用于管理 TIM6 的结构体
TIM_HandleTypeDef g_tim7_handle;   // 同上，TIM7
```

每个定时器需要一个 HAL 句柄，保存它的配置和状态。

---

## 2.2 `btim_tim6_int_init()`（第 9 ~ 18 行）

```c
void btim_tim6_int_init(uint16_t arr, uint16_t psc)
{
    g_tim6_handle.Instance = BTIM_TIM6_INT;               // ① 绑定到 TIM6 外设
    g_tim6_handle.Init.Prescaler = psc;                   // ② 预分频系数
    g_tim6_handle.Init.CounterMode = TIM_COUNTERMODE_UP;  // ③ 向上计数
    g_tim6_handle.Init.Period = arr;                      // ④ 自动重装载值（上限）
    HAL_TIM_Base_Init(&g_tim6_handle);                    // ⑤ 初始化 → 内部调用 HAL_TIM_Base_MspInit

    HAL_TIM_Base_Start_IT(&g_tim6_handle);                // ⑥ 启动定时器 + 使能更新中断
}
```

**定时器中断频率公式**：

```plain text
arr = 10000 - 1
psc = 7200 - 1

定时器时钟 = 72MHz / psc = 72,000,000 / 7200 = 10,000 Hz = 10KHz
中断频率   = 10,000 / arr = 10,000 / 10,000 = 1 Hz = 每秒 1 次
```

**流程**：计数器从 0 往上数 → 数到 9999 → 溢出回 0，触发一次更新中断 → 循环。

`btim_tim7_int_init()` 同理，只是操作 TIM7。

---

## 2.3 `HAL_TIM_Base_MspInit()`（第 36 ~ 51 行）— 底层硬件初始化

```c
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == BTIM_TIM6_INT)         // 判断是哪个定时器调用的
    {
        BTIM_TIM6_INT_CLK_ENABLE();              // ┐ 开时钟
        HAL_NVIC_SetPriority(BTIM_TIM6_INT_IRQn, 6, 0);  // │ 抢占优先级 6
        HAL_NVIC_EnableIRQ(BTIM_TIM6_INT_IRQn);          // ┘ 使能 NVIC
    }

    if (htim->Instance == BTIM_TIM7_INT)
    {
        BTIM_TIM7_INT_CLK_ENABLE();
        HAL_NVIC_SetPriority(BTIM_TIM7_INT_IRQn, 4, 0);  // ★ 抢占优先级 4
        HAL_NVIC_EnableIRQ(BTIM_TIM7_INT_IRQn);
    }
}
```

**这是本实验最核心的设置**：

| 定时器 | NVIC 抢占优先级 | 含义 |
| --- | --- | --- |
| TIM7 | **4** | 优先级高于 5 → 不在 FreeRTOS 管理范围内 |
| TIM6 | **6** | 优先级低于 5 → 在 FreeRTOS 管理范围内 |

**MspInit 调用链**：`HAL_TIM_Base_Init()` → 内部发现 MspInit 未执行 → 自动调用 `HAL_TIM_Base_MspInit()`。这是 HAL 库的"分层初始化"设计：`HAL_TIM_Base_Init` 负责逻辑初始化，`MspInit` 负责硬件层（时钟、NVIC、GPIO）。

---

## 2.4 中断服务函数（第 58 ~ 65 行）

```c
void BTIM_TIM6_INT_IRQHandler(void)    // 等价于 void TIM6_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_tim6_handle);  // HAL 通用处理 → 清标志 → 调回调
}

void BTIM_TIM7_INT_IRQHandler(void)    // 等价于 void TIM7_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_tim7_handle);
}
```

**这是 ARM 启动文件里向量表指向的入口**。HAL 层的 `HAL_TIM_IRQHandler` 负责：清中断标志位 → 判断中断类型（更新/捕获/比较）→ 调用对应的回调。

---

## 2.5 `HAL_TIM_PeriodElapsedCallback()`（第 72 ~ 85 行）— 中断回调

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static uint32_t num1 = 0;    // static: 计数跨中断保留
    static uint32_t num2 = 0;

    if (htim->Instance == BTIM_TIM6_INT)
    {
        printf("TIM6优先级为6的正在运行!!!,num1 = %d\\r\\n", ++num1);
        LED0_TOGGLE();
    }
    else if(htim->Instance == BTIM_TIM7_INT)
    {
        printf("TIM7优先级为4的正在运行!!!!!!!!!num2 = %d\\r\\n", ++num2);
        LED1_TOGGLE();
    }
}
```

**函数名拆解**：`HAL` + `TIM` + `PeriodElapsed`（周期结束）+ `Callback`（回调）= 定时器周期溢出回调。

两个定时器共享同一个回调，通过 `htim->Instance` 判断是谁触发的。

---

## 三、`freertos_dome.c` — 任务主体

## 3.1 宏定义 + 句柄（第 11 ~ 20 行）

```c
#define START_TASK_PRIO         1
#define START_TASK_STACK_SIZE   128
TaskHandle_t start_task_handle;
void freertos_dome(void);

#define TASK1_PRIO         2
#define TASK1_STACK_SIZE   128
TaskHandle_t task1_handle;
void start_task(void *pvParameters);
void task1(void *pvParameters);
```

只有 2 个任务：`start_task`（创建子任务用）和 `task1`（核心功能）。

---

# 3.2 `freertos_dome()` — 入口（第 22 ~ 31 行）

```c
void freertos_dome(void)
{
    xTaskCreate(start_task, "start_task", 128, NULL, 1, &start_task_handle);
    vTaskStartScheduler();
}
```

和前面实验完全一样：创建 start_task → 启动调度器。

---

## 3.3 `start_task()` — 创建 task1 后自杀（第 33 ~ 45 行）

```c
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();
    xTaskCreate(task1, "task1", 128, NULL, 2, &task1_handle);
    taskEXIT_CRITICAL();
    vTaskDelete(NULL);
}
```

---

## 3.4 `task1()` — 本实验核心逻辑（第 47 ~ 63 行）

```c
void task1(void *pvParameters)
{
    uint8_t task1_num = 0;
    while(1)
    {
        if(++task1_num == 5)           // ① 每 5 次循环触发一次
        {
            task1_num = 0;             // ② 计数归零
            printf("关中断！！\\r\\n");
            portDISABLE_INTERRUPTS();  // ③ ★ 屏蔽 FreeRTOS 管理范围内的中断
            delay_ms(5000);            // ④ 延时 5 秒（CPU 不阻塞，定时器还跑）
            printf("开中断！！！\\r\\n");
            portENABLE_INTERRUPTS();   // ⑤ ★ 恢复
        }
        vTaskDelay(1000);              // ⑥ 阻塞 1 秒
    }
}
```

**时序**：

```plain text
时间轴：
  0s: task1 运行，task1_num=1，vTaskDelay(1000) 阻塞
  1s: task1 运行，task1_num=2，vTaskDelay(1000) 阻塞
  2s: task1 运行，task1_num=3，vTaskDelay(1000) 阻塞
  3s: task1 运行，task1_num=4，vTaskDelay(1000) 阻塞
  4s: task1 运行，task1_num=5 → 关中断！
       ┌──────── 5 秒屏蔽窗口 ────────┐
       │ TIM6(优先级6) 被屏蔽 ❌       │
       │ TIM7(优先级4) 不受影响 ✅     │
       └──────────────────────────────┘
  9s: 开中断！task1_num=0，继续循环
```

---

# 四、`FreeRTOSConfig.h` — 中断管理关键配置

## 4.1 中断优先级位（第 94 ~ 105 行）

```c
#define configPRIO_BITS                      4        // STM32 使用 4 位优先级（0~15）

/* STM32 库函数的优先级（0~15） */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      15   // 最低优先级
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5    // ★ FreeRTOS 能管理的最高优先级

/* 转换为寄存器原始值（左移到高 4 位） */
#define configKERNEL_INTERRUPT_PRIORITY             (15 << 4)   // = 0xF0
#define configMAX_SYSCALL_INTERRUPT_PRIORITY        (5 << 4)    // = 0x50
```

**这是理解整个实验的关键**：

```plain text
STM32 优先级编号（0最高，15最低）：

  优先级   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
           ▲                      ▲  ▲
           │                      │  │
   最高优先级                TIM7(4) TIM6(6)
                               │        │
                               │        └── ≥ 5 → FreeRTOS 管得到
                               └── < 5 → FreeRTOS 管不到！
```

`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5` 的含义：

> **优先级号 ≥ 5 的中断**（即优先级 5~15，较低优先级）→ FreeRTOS 可以管理它们（屏蔽、在中断里调 API）
> **优先级号 < 5 的中断**（即优先级 0~4，较高优先级）→ FreeRTOS 完全不管，也不能在其中调用任何 FreeRTOS API

---

## 4.2 中断 API 使能（第 80、92 行）

```c
#define INCLUDE_xResumeFromISR           1    // 允许在中断中恢复任务
#define INCLUDE_xTaskResumeFromISR       1    // 同上
```

---

## 4.3 其他与本实验相关的配置

```c
#define configTICK_RATE_HZ               1000    // 系统滴答 1000Hz = 1ms 一次
#define configMAX_PRIORITIES             32      // 最多 32 个优先级
#define configUSE_PREEMPTION             1       // 抢占式调度
```

---

# 五、`main.c` — 启动流程

```c
int main(void)
{
    HAL_Init();                                 // ① HAL 库初始化
    sys_stm32_clock_init(RCC_PLL_MUL9);        // ② 系统时钟 72MHz
    delay_init(180);                            // ③ 延时初始化
    usart_init(115200);                         // ④ 串口 115200
    led_init();                                 // ⑤ LED 初始化
    key_init();                                 // ⑥ 按键初始化
    btim_tim6_int_init(10000-1, 7200-1);       // ⑦ TIM6 初始化：1秒触发一次
    btim_tim7_int_init(10000-1, 7200-1);       // ⑧ TIM7 初始化：1秒触发一次

    freertos_dome();                             // ⑨ 进入 FreeRTOS
}
```

---

# 六、本实验核心知识点总结

## 6.1 `portDISABLE_INTERRUPTS()` 到底屏蔽了什么？

```c
portDISABLE_INTERRUPTS();
// 内部实现（Cortex-M3）：
//   ulMask = configMAX_SYSCALL_INTERRUPT_PRIORITY;  // = 0x50
//   __asm volatile ("msr basepri, %0" : : "r"(ulMask));
//
// 效果：设置 BASEPRI = 0x50
//   优先级 >= 0x50（即库优先级 5~15）→ 被屏蔽 ❌
//   优先级 <  0x50（即库优先级 0~4） → 不受影响 ✅
```

```plain text
┌─────────────────────────────────────────────────┐
│                                                 │
│  优先级 0~4  被屏蔽？ NO  → 可以打断任何代码    │ ← TIM7(4) 在这儿
│  ─────────────────────────────                  │
│  优先级 5~15 被屏蔽？ YES → 不能打断当前代码    │ ← TIM6(6) 在这儿
│                                                 │
│  SysTick/PendSV/SVC 的优先级通常设为 15         │
│  也被屏蔽 → FreeRTOS 调度暂停                    │
└─────────────────────────────────────────────────┘
```

## 6.2 实验现象

```plain text
正常时（task1 没关中断，task1_num < 5）：
  TIM6优先级为6的正在运行!!!,num1 = 1    ← 每秒交替
  TIM7优先级为4的正在运行!!!!!!!!!num2 = 1
  TIM6优先级为6的正在运行!!!,num1 = 2
  TIM7优先级为4的正在运行!!!!!!!!!num2 = 2
  ...

关中断 5 秒期间：
  关中断！！
  TIM7优先级为4的正在运行!!!!!!!!!num2 = 3   ← 继续中断 ✅
  TIM7优先级为4的正在运行!!!!!!!!!num2 = 4
  TIM7优先级为4的正在运行!!!!!!!!!num2 = 5
  TIM7优先级为4的正在运行!!!!!!!!!num2 = 6
  TIM7优先级为4的正在运行!!!!!!!!!num2 = 7
  (TIM6 消失不见了 ❌)
  开中断！！！
  TIM6优先级为6的正在运行!!!,num1 = 3   ← 恢复！
  TIM7优先级为4的正在运行!!!!!!!!!num2 = 8
```

**结论**：`portDISABLE_INTERRUPTS()` 不是关所有中断，而是关"FreeRTOS 能管的中断"。TIM7 优先级太高，FreeRTOS 管不着，所以你关不掉。

## 6.3 设计原则

```plain text
┌──────────────────────────────────────────────────────┐
│  需要使用 FreeRTOS API 的中断（如队列、信号量等）     │
│  → 优先级设为 5~15                                   │
│  → 可以安全调用 xQueueSendFromISR 等                  │
│  → 可以被 portDISABLE_INTERRUPTS 屏蔽                │
├──────────────────────────────────────────────────────┤
│  绝对实时性要求的中断（如电机控制、紧急停机）         │
│  → 优先级设为 0~4                                    │
│  → 完全不受 FreeRTOS 干扰                            │
│  → 禁止调用任何 FreeRTOS API！                       │
│  → ISR 必须极短，做最少的事情                         │
└──────────────────────────────────────────────────────┘
```

**一句话**：FreeRTOS 不是真正的实时操作系统，它只能管理优先级 5~15 的中断。0~4 的中断才是真正的硬实时。