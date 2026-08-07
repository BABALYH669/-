---
notion-id: 37a5d378-8ef5-80bf-8f0f-fa09d7cd00ba
---
# 任务的挂起与恢复的API函数

| API函数 | 描述 |
| --- | --- |
| vTaskSuspend() | 挂起任务 |
| vTaskResume() | 恢复被挂起的任务 |
| xTaskResumeFromISR() | 在中断中恢复被挂起的任务 |

![[wps_IyKiLfrXr8.png|500]]

![[wps_sJCsVPV0lC.png|525]]

![[wps_FTUfXA2zAn.png|525]]

挂起:类似于暂停,可恢复    删除:无法恢复,清除堆栈
恢复:恢复被挂起的任务
FromISR带有个后缀是在中断函数中专用的API函数
FreeRTOS     中数值越大,任务优先级越高
NVIC(中断)  中数值越小,任务优先级越高


# 任务挂起与恢复实验

# 一、`freertos_dome.c` — 任务主体

## 整体架构：4 个任务的协作关系

```plain text
优先级 4  task3 ── KEY0 → 挂起 task1 / task2
                      KEY1 → 恢复 task1 / task2（任务上下文）

优先级 3  task2 ── LED1 闪烁 + 串口计数

优先级 2  task1 ── LED0 闪烁 + 串口计数

优先级 1  start_task ── 创建子任务后自杀

优先级 0  空闲任务 ── 所有任务阻塞时运行
```

---

## 第 1 步：头文件 + 宏定义 + 句柄声明（第 1 ~ 29 行）

```c
#include "./SYSTEM/usart/usart.h"   // 串口驱动（printf）
#include "./BSP/LED/led.h"          // LED0_TOGGLE / LED1_TOGGLE
#include "./BSP/KEY/key.h"          // key_scan / KEY0_PRES / KEY1_PRES
#include "freertos_dome.h"          // 本文件头文件
#include "FreeRTOS.h"               // FreeRTOS 核心
#include "task.h"                   // 任务 API（xTaskCreate / vTaskDelete / vTaskSuspend / vTaskResume）

/* ====== 4 个任务 ====== */
#define START_TASK_PRIO         1          // 优先级最低的用户任务
#define START_TASK_STACK_SIZE   128
TaskHandle_t start_task_handle;
void freertos_dome(void);                  // 入口函数声明
void start_task(void *pvParameters);       // 启动作务函数声明

#define TASK1_PRIO         2
#define TASK1_STACK_SIZE   128
TaskHandle_t task1_handle;
void task1(void *pvParameters);

#define TASK2_PRIO         3
#define TASK2_STACK_SIZE   128
TaskHandle_t task2_handle;
void task2(void *pvParameters);

#define TASK3_PRIO         4              // 优先级最高
#define TASK3_STACK_SIZE   128
TaskHandle_t task3_handle;
void task3(void *pvParameters);
```

**4 个任务的句柄（handle）都是全局变量**，task3 用它来挂起/恢复 task1 和 task2，`exti.c` 也用它从中断里恢复任务。

---

## 第 2 步：入口函数 `freertos_dome()`（第 31 ~ 40 行）

```c
void freertos_dome(void)
{
    //  ① 任务函数      ② 名称        ③ 栈深度  ④ 参数  ⑤ 优先级  ⑥ 句柄
    xTaskCreate(start_task, "start_task", 128,     NULL,   1,       &start_task_handle);
    vTaskStartScheduler();  // 启动调度器，CPU 从此由 FreeRTOS 接管
}
```

**启动流程**：创建 `start_task` → 启动调度器 → 调度器选中 `start_task`（它是唯一的就绪用户任务）→ 开始运行。

---

## 第 3 步：`start_task` — 创建子任务后自杀（第 42 ~ 67 行）

```c
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();        // ┐ 关中断/禁调度
    xTaskCreate(task1, "task1", 128, NULL, 2, &task1_handle);  // 创建 task1
    xTaskCreate(task2, "task2", 128, NULL, 3, &task2_handle);  // 创建 task2
    xTaskCreate(task3, "task3", 128, NULL, 4, &task3_handle);  // 创建 task3
    taskEXIT_CRITICAL();         // ┘ 恢复

    vTaskDelete(NULL);           // 删除自己（传 NULL = 删除调用者自身）
}
```

| 关键点 | 说明 |
| --- | --- |
| `taskENTER_CRITICAL()` | 关中断，确保三个任务原子创建，防止只创建到一半就被抢占 |
| `vTaskDelete(NULL)` | 传 `NULL` 表示"删自己"，start_task 使命完成，不再占用内存 |
| 创建完 task3 后 | task3 优先级最高(4)，调度器在 `taskEXIT_CRITICAL` 后立刻切到 task3 |

---

## 第 4 步：`task1` — 自增计数 + LED0 翻转（第 69 ~ 78 行）

```c
void task1(void *pvParameters)
{
    uint32_t task1_num = 0;          // 局部变量，存在任务栈里
    while(1)
    {
        printf("task1_num = %d\\r\\n", ++task1_num);  // 先 +1 再打印
        LED0_TOGGLE();               // LED0 翻转
        vTaskDelay(500);             // 阻塞 500ms，让 CPU 去跑别的任务
    }
}
```

```plain text
输出示例：task1_num = 1  →  task1_num = 2  →  task1_num = 3  → ...
```

---

## 第 5 步：`task2` — 同理，LED1（第 80 ~ 89 行）

```c
void task2(void *pvParameters)
{
    uint32_t task2_num = 0;
    while(1)
    {
        printf("task2_num = %d\\r\\n", ++task2_num);
        LED1_TOGGLE();
        vTaskDelay(500);
    }
}
```

---

## 第 6 步：`task3` — 按键控制挂起/恢复（第 91 ~ 158 行）

这是整个文件的核心逻辑，分三部分讲解。

### 6.1 全局变量（存在任务栈 vs 静态）

```c
    uint8_t key = 0;                  // 普通局部：每次循环被重新赋值为 key_scan 的返回值
    static uint8_t press_count_key0 = 0;  // static：只初始化一次，值跨循环保留
    static uint8_t press_count_key1 = 0;  // static：同上
```

| 变量 | 存储位置 | 生命周期 |
| --- | --- | --- |
| `key` | task3 的栈 | 每次循环可用，循环结束释放 |
| `press_count_key0` | 全局静态区 | 整个程序生命周期，值不丢失 |

### 6.2 KEY0 — 挂起（第 101 ~ 127 行）

```c
if(key == KEY0_PRES)           // ① 检测 KEY0 是否按下
{
    press_count_key0++;        // ② 计数 +1
    if(press_count_key0 > 2)   // ③ 超过 2 次回绕到 1
    {
        press_count_key0 = 1;
    }

    if(press_count_key0 == 1)           // ④ 第 1 次按 → 挂起 task1
    {
        if(task1_handle != NULL)        // ⑤ 安全校验：task1 还活着？
        {
            printf("挂起task1\\r\\n");
            vTaskSuspend(task1_handle); // ⑥ 挂起！
        }
    }
    else if(press_count_key0 == 2)      // ⑦ 第 2 次按 → 挂起 task2
    {
        if(task2_handle != NULL)
        {
            printf("挂起task2\\r\\n");
            vTaskSuspend(task2_handle);
        }
    }
}
// 没有按键 → 跳过整个 if 块，什么都不做
```

```plain text
按键时序：
  第 1 次 KEY0 → press_count_key0: 0→1 → 挂起 task1
  第 2 次 KEY0 → press_count_key0: 1→2 → 挂起 task2
  第 3 次 KEY0 → press_count_key0: 2→3→1 → 挂起 task1（循环）
```

**为什么不会反复执行？** 因为挂起操作写在 `if(key == KEY0_PRES)` 里面。只有按下按键的那个循环周期才执行，接下来 10ms 后下一个循环，`key` 变成了 0（无按键），整个 `if` 跳过，安静等待下一次按键。

### 6.3 KEY1 — 恢复（第 128 ~ 154 行）

```c
else if(key == KEY1_PRES)
{
    press_count_key1++;
    if(press_count_key1 > 2)
    {
        press_count_key1 = 1;
    }

    if(press_count_key1 == 1)
    {
        if(task1_handle != NULL)
        {
            printf("恢复task1\\r\\n");
            vTaskResume(task1_handle);   // ← 任务上下文中使用 vTaskResume
        }
    }
    else if(press_count_key1 == 2)
    {
        if(task2_handle != NULL)
        {
            printf("恢复task2\\r\\n");
            vTaskResume(task2_handle);
        }
    }
}
```

**注意**：这里用的是 `else if`，意味着同一个循环周期内 KEY0 和 KEY1 不会同时响应（只有一个是真）。

---

## 第 7 步：`vTaskDelay(10)`（第 156 行）

```c
vTaskDelay(10);  // 阻塞 10ms，让出 CPU
```

task3 每个循环只执行约 10µs，然后阻塞 10ms。**任务占用 CPU 比例不到 0.1%**，其他时间 CPU 在运行 task1 和 task2。

---

# 二、`exti.c` — 外部中断（中断中恢复任务）

---

## 第 1 步：头文件 + 外部变量声明（第 1 ~ 11 行）

```c
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/KEY/key.h"
#include "./BSP/EXTI/exti.h"
#include "freertos_dome.h"          // 引入 FreeRTOS API
#include "FreeRTOS.h"
#include "task.h"

extern TaskHandle_t task1_handle;   // 声明：这两个句柄定义在 freertos_dome.c 里
extern TaskHandle_t task2_handle;   // extern = 告诉编译器"去别的 C 文件找"
```

`extern` 使得本文件能访问 `freertos_dome.c` 中定义的全局变量 `task1_handle` 和 `task2_handle`。

---

## 第 2 步：中断服务函数 `WKUP_INT_IRQHandler`（第 18 ~ 22 行）

```c
void WKUP_INT_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(WKUP_INT_GPIO_PIN);   // ① HAL 公用处理（清中断标志 + 调 callback）
    __HAL_GPIO_EXTI_CLEAR_IT(WKUP_INT_GPIO_PIN);   // ② 再清一次中断标志，防抖动误触发
}
```

**这是 ARM Cortex-M 的真正中断入口**。

```plain text
硬件 WK_UP 引脚上升沿
    │
    ▼
NVIC 查向量表 → 跳到 WKUP_INT_IRQHandler
    │
    ├─ HAL_GPIO_EXTI_IRQHandler()
    │     ├─ 判断中断标志位
    │     ├─ 清除中断标志
    │     └─ 调用 HAL_GPIO_EXTI_Callback(GPIO_Pin)  ← 你的代码在这里
    │
    └─ __HAL_GPIO_EXTI_CLEAR_IT() → 再清一次，避免按键抖动产生二次中断
```

---

## 第 3 步：`HAL_GPIO_EXTI_Callback` — 中断中恢复任务（第 30 ~ 73 行）

### 3.1 变量声明

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xYieldRequired = pdFALSE;    // ① 切换标志，初始 = 不需要
    static uint8_t wkup_press_count = 0;    // ② 按键次数计数（静态保留）
```

| 变量 | 说明 |
| --- | --- |
| `xYieldRequired` | 返回给 FreeRTOS 的"是否立即切换"标志。初始 `pdFALSE`（不切换），如果恢复了一个高优先级任务则变为 `pdTRUE` |
| `wkup_press_count` | 和 task3 里一样的计数逻辑：1 → 恢复 task1，2 → 恢复 task2 |

---

### 3.2 Switch 分支

```c
    switch(GPIO_Pin)                       // ③ 判断是哪个引脚触发的中断
    {
        case WKUP_INT_GPIO_PIN:            // ④ 是 WK_UP 这个引脚
            delay_ms(20);                  // ⑤ 软件消抖 20ms
            if (WK_UP == 1)                // ⑥ 消抖后再读一次，确认真的按下了
            {
```

| 步骤 | 操作 | 原因 |
| --- | --- | --- |
| `switch(GPIO_Pin)` | 多引脚共用回调的分流 | 同一个 `HAL_GPIO_EXTI_Callback` 可能被多个引脚复用 |
| `delay_ms(20)` | 等待 20ms | 避开按键机械弹跳期 |
| `if (WK_UP == 1)` | 再次读取电平 | 20ms 后如果还是高，是真按；如果变低了，是干扰 |

---

### 3.3 计数 + 恢复操作

```c
                wkup_press_count++;        // ⑦ 计数 +1
                if(wkup_press_count > 2)   // ⑧ 循环收束 1↔2
                {
                    wkup_press_count = 1;
                }

                if(wkup_press_count == 1)       // ⑨ 第 1 次 → 恢复 task1
                {
                    if(task1_handle != NULL)    // ⑩ 安全检查
                    {
                        printf("在中断中恢复task1\\r\\n");
                        xYieldRequired = xTaskResumeFromISR(task1_handle); // ⑪ 中断版恢复
                    }
                }
                else if(wkup_press_count == 2)  // ⑫ 第 2 次 → 恢复 task2
                {
                    if(task2_handle != NULL)
                    {
                        printf("在中断中恢复task2\\r\\n");
                        xYieldRequired = xTaskResumeFromISR(task2_handle);
                    }
                }
```

`**xTaskResumeFromISR**`** vs **`**vTaskResume**`：

|   | 任务上下文 | 中断上下文 |
| --- | --- | --- |
| 恢复任务 | `vTaskResume(handle)` | `xTaskResumeFromISR(handle)` |
| 返回值 | 无 | `pdTRUE`（需要立即切换）/ `pdFALSE`（不需要） |

**中断里不能用普通版**。普通版会调用可能导致阻塞的 API，中断里一阻塞就死机。中断版只做标记然后返回，由调度器处理。

---

### 3.4 统一任务切换（第 68 ~ 72 行）

```c
    if(xYieldRequired == pdTRUE)
    {
        portYIELD_FROM_ISR(xYieldRequired);
    }
```

**放在 switch 外面统一判断，而不是在每个 case 里重复写。**

```plain text
xYieldRequired == pdTRUE 的场景：
  当前正在运行 task3（优先级 4），WK_UP 中断来了
  → xTaskResumeFromISR 恢复了 task1（优先级 2）
  → 恢复的 task1 优先级(2) < 当前被打断的 task3(4)
  → 不需要立即切换，xYieldRequired = pdFALSE
  → 中断返回，继续跑 task3

xYieldRequired == pdTRUE 的场景：
  当前正在运行 task1（优先级 2），WK_UP 中断来了
  → xTaskResumeFromISR 恢复了 task2（优先级 3）
  → 恢复的 task2 优先级(3) > 当前被打断的 task1(2)
  → 需要立即切换！xYieldRequired = pdTRUE
  → portYIELD_FROM_ISR 触发 PendSV
  → 中断返回时直接切到 task2，不等下一次 SysTick
```

---

## 第 4 步：`extix_init()` — 中断初始化（第 80 ~ 93 行）

```c
void extix_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    WKUP_GPIO_CLK_ENABLE();                    // ① 使能 GPIO 时钟

    gpio_init_struct.Pin  = WKUP_INT_GPIO_PIN;  // ② 引脚
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING; // ③ 上升沿触发（按下 = 低→高）
    gpio_init_struct.Pull = GPIO_PULLDOWN;      // ④ 下拉（不按时保持低电平）
    HAL_GPIO_Init(WKUP_GPIO_PORT, &gpio_init_struct);

    HAL_NVIC_SetPriority(WKUP_INT_IRQn, 5, 0);  // ⑤ 抢占优先级 5，子优先级 0
    HAL_NVIC_EnableIRQ(WKUP_INT_IRQn);          // ⑥ 在 NVIC 中使能中断
}
```

| 步骤 | 含义 |
| --- | --- |
| ① 时钟使能 | 所有 STM32 外设第一步：开时钟 |
| ② 引脚 | WK_UP 对应的 GPIO 引脚（通常是 PA0） |
| ③ 上升沿触发 | WK_UP 按下时电平从低变高，产生中断 |
| ④ 下拉 | 不按时默认拉低到 GND，防止悬空误触发 |
| ⑤ 优先级 5,0 | 比 FreeRTOS 管理的优先级（`configMAX_SYSCALL_INTERRUPT_PRIORITY`）低 → 中断里可以安全调用 FreeRTOS 的 FromISR API |
| ⑥ 使能 | 允许 NVIC 响应此中断 |

---

## 完整运行流程（两个文件配合）

```plain text
main()
  └─ 调用 extix_init()         // ← exti.c：配置 WK_UP 中断
  └─ 调用 freertos_dome()      // ← freertos_dome.c：创建任务 + 启动调度器
        │
        ├─ start_task 创建 task1、task2、task3 → 自杀
        │
        ├─ task3 (优先级4) 每 10ms 扫描 KEY0/KEY1
        │     │
        │     ├─ [KEY0 按下] → 计数 → vTaskSuspend(task1) 或 task2
        │     └─ [KEY1 按下] → 计数 → vTaskResume(task1) 或 task2
        │
        ├─ task2 (优先级3) 每 500ms LED1 闪烁 + 计数++
        │
        ├─ task1 (优先级2) 每 500ms LED0 闪烁 + 计数++
        │
        └─ [WK_UP 按下 硬件中断]       // ← exti.c
              └─ HAL_GPIO_EXTI_Callback
                    └─ 计数 → xTaskResumeFromISR(task1) 或 task2
                    └─ 如果恢复的任务优先级更高 → 立刻切换
```

---

## 两个恢复路径对比

| 对比项 | KEY1 → task3 恢复 | WK_UP → 中断恢复 |
| --- | --- | --- |
| **触发方式** | 轮询扫描 `key_scan()` | 硬件中断（上升沿） |
| **上下文** | 任务上下文 | 中断上下文 |
| **API** | `vTaskResume(handle)` | `xTaskResumeFromISR(handle)` |
| **恢复时机** | 下次 task3 运行时（最多 10ms） | 中断退出后立即（微秒级） |
| **能否抢占低优先级** | 不能，调度器下次切换才生效 | 能，`portYIELD_FROM_ISR` 立刻切 |
| **文件** | `freertos_dome.c` | `exti.c` |
| **按键** | KEY0（挂起）/ KEY1（恢复） | WK_UP（只恢复） |