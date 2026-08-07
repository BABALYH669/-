---
notion-id: 3795d378-8ef5-80da-ba2e-d83fdfbabd85
---
# FreeRTOS的任务创建和删除

## 任务创建和删除的API函数

| API函数 | 描述 |
| --- | --- |
| xTaskCreat() | 动态方式创建任务 |
| xTaskCreateStatic() | 静态方式创建任务 |
| vTaskDelete() | 删除任务 |

**动态创建任务**:任务的任务控制块以及任务的栈空间所需的内存，均由 FreeRTOS 从 FreeRTOS 管理的堆中分配

![[wps_m6ZhROomcY.png]]

![[wps_HXg47lZhKB.png]]

![[wps_frJ3pGQm3d.png]]

volatile StackType_t  *pxTopOFStack  任务切换,超级重要

**静态创建任务:**任务的任务控制块以及任务的栈空间所需的内存，需用户分配提供

![[wps_mI2sBFQSoc.png]]

![[wps_3hJBTBP70D.png]]

**任务删除函数**

![[wps_02R0NYcZ9y.png]]

在任务一中调用删除函数,删除的是任务二,释放内存空间就在这个函数调用处
在任务一中调用删除函数,传入的是NULL是删除自身,释放内存空间就不会在函数调用处了,会在空闲任务去释放
静态需要自己手动释放

![[wps_tzHaf7pMiF.png]]

**
**

# 任务创建和删除(动态方法)

## `freertos_dome.c` 详细步骤讲解

---

### 第 1 步：引入硬件驱动头文件（第 1 ~ 3 行）

```c
#include "./SYSTEM/usart/usart.h"   // 串口驱动 → 让 printf 能通过串口输出
#include "./BSP/LED/led.h"          // LED 驱动 → LED0_TOGGLE()、LED1_TOGGLE()
#include "./BSP/KEY/key.h"          // 按键驱动 → key_scan()、KEY0_PRES
```

把需要用到的硬件驱动全部包含进来，后面才能用它们提供的函数。

---

### 第 2 步：引入 FreeRTOS 头文件（第 5 ~ 6 行）

```c
#include "FreeRTOS.h"     // ← 必须在 task.h 之前！
#include "task.h"         // ← 任务管理 API：xTaskCreate、vTaskDelete、vTaskDelay 等
```

**顺序不能错**：`task.h` 依赖 `FreeRTOS.h` 里定义的基础类型和宏，倒过来就会编译报错。

---

### 第 3 步：定义"启动任务"的配置参数（第 9 ~ 12 行）

```c
#define START_TASK_PRIO         1       // 优先级 = 1（数字越小优先级越低）
#define START_TASK_STACK_SIZE   128     // 栈大小 128 个字（32 位 MCU 上 = 512 字节）
TaskHandle_t start_task_handle;         // 任务句柄（遥控器），后面通过它控制这个任务
void freertos_dome(void);               // 前向声明：告诉编译器这个函数存在
```

**解释**：

- **优先级**：FreeRTOS 中数字越大优先级越高，这里 1 是最低的，所以启动任务不会抢其他任务的时间。
- **栈大小**：每个任务有自己独立的栈，128 个字大约 512 字节，够这个任务用。
- **句柄**：相当于任务的"身份证号"或"遥控器"，创建任务时把句柄地址传进去，FreeRTOS 会把任务 ID 写回来。

---

### 第 4 步：定义三个子任务的配置参数（第 14 ~ 30 行）

```c
#define TASK1_PRIO         2          // task1 优先级 2
#define TASK1_STACK_SIZE   128        // 栈 128 字
TaskHandle_t task1_handle;            // task1 的句柄

#define TASK2_PRIO         3          // task2 优先级 3（比 task1 高）
TaskHandle_t task2_handle;

#define TASK3_PRIO         4          // task3 优先级 4（最高！）
TaskHandle_t task3_handle;
```

优先级排行：**task3(4) > task2(3) > task1(2) > start_task(1)**

task3 优先级最高，意味着只要 task3 就绪，它就第一个跑。

---

### 第 5 步：前向声明四个任务函数（第 17、19、23、27 行）

```c
void start_task(void *pvParameters);   // 启动任务
void task1(void *pvParameters);        // LED0 闪烁任务
void task2(void *pvParameters);        // LED1 闪烁任务
void task3(void *pvParameters);        // 按键扫描任务
```

**为什么需要？** C 语言是顺序编译的——函数定义写在使用它的代码后面时，编译器不认识它。前向声明相当于提前说："有个函数叫 `task1`，参数长这样，后面会定义。"

---

### 第 6 步：`freertos_dome()` — 系统初始化和启动（第 33 ~ 42 行）

```c
void freertos_dome(void)
{
    // 6-1：创建"启动任务"
    xTaskCreate(
        (TaskFunction_t)            start_task,           // 要运行的函数
        (char *)                    "start_task",         // 任务名字（调试用）
        (configSTACK_DEPTH_TYPE)    START_TASK_STACK_SIZE,// 栈大小
        (void *)                    NULL,                 // 不传参数
        (UBaseType_t)               START_TASK_PRIO,      // 优先级
        (TaskHandle_t *)            &start_task_handle    // 写回句柄
    );

    // 6-2：启动 FreeRTOS 调度器（这个函数永远不会返回！）
    vTaskStartScheduler();
}
```

**执行过程**：

1. `xTaskCreate` 在 FreeRTOS 内核里注册了一个任务，任务函数是 `start_task`，优先级 1。
2. `vTaskStartScheduler()` 启动调度器，从此 CPU 的控制权交给 FreeRTOS，调度器开始按优先级轮流执行任务。
3. **注意**：调用 `xTaskCreate` 只是"注册"，任务还没开始跑。`vTaskStartScheduler()` 之后，调度器看到 `start_task` 是唯一就绪的任务，才会启动它。

---

### 第 7 步：`start_task()` — 启动任务（第 44 ~ 69 行）

```c
void start_task(void *pvParameters)
{
    // 7-1：进入临界区（关中断）
    taskENTER_CRITICAL();

    // 7-2：一口气创建 3 个子任务
    xTaskCreate(..., task1, "task1", 128, NULL, 2, &task1_handle);
    xTaskCreate(..., task2, "task2", 128, NULL, 3, &task2_handle);
    xTaskCreate(..., task3, "task3", 128, NULL, 4, &task3_handle);

    // 7-3：退出临界区（开中断）
    taskEXIT_CRITICAL();

    // 7-4：删除自己（启动任务使命完成，自杀）
    vTaskDelete(NULL);
}
```

**执行过程**：

4. **进临界区**：`taskENTER_CRITICAL()` 关掉中断，保证创建三个任务的过程不被打断。
5. **创建子任务**：连续调用 3 次 `xTaskCreate`，在内核中注册 `task1`、`task2`、`task3`。此时它们都处于"就绪态"。
6. **退临界区**：`taskEXIT_CRITICAL()` 重新打开中断。
7. **自杀**：`vTaskDelete(NULL)` 删除当前任务（`start_task` 自己）。`NULL` 表示"删自己"。这个函数不会返回——调用后，`start_task` 就永远消失了。

**为什么要自殺？** `start_task` 的唯一工作就是创建那 3 个子任务，干完活了就没必要存在了，删掉省内存、省调度开销。

---

### 第 8 步：`task1()` — LED0 闪烁任务（第 71 ~ 79 行）

```c
void task1(void *pvParameters)
{
    while(1)                          // 8-1：死循环，任务永远不退出
    {
        printf("task1正在运行\\r\\n");    // 8-2：串口打印 "task1正在运行"
        LED0_TOGGLE();                 // 8-3：翻转 LED0（亮 → 灭 → 亮 → 灭）
        vTaskDelay(500);               // 8-4：阻塞 500ms，让出 CPU
    }
}
```

**执行过程**：

8. 进入 `while(1)` 死循环，FreeRTOS 的任务绝不能退出（退出会崩溃）。
9. 串口输出 "task1正在运行"。
10. 翻转 LED0 状态。
11. `**vTaskDelay(500)**`：这是关键——任务调用 `vTaskDelay` 后进入**阻塞态**，把 CPU 让给其他任务。500 个时钟节拍后（通常 1 节拍 = 1ms，即 500ms），调度器把它唤醒，重新回到 `while` 循环顶部。

---

### 第 9 步：`task2()` — LED1 闪烁任务（第 81 ~ 89 行）

```c
void task2(void *pvParameters)
{
    while(1)
    {
        printf("task2正在运行\\r\\n");
        LED1_TOGGLE();                 // 翻转 LED1
        vTaskDelay(500);               // 同样阻塞 500ms
    }
}
```

和 `task1` 完全一样，只是操作 LED1 而不是 LED0。

---

### 第 10 步：`task3()` — 按键扫描任务（第 91 ~ 109 行）

```c
void task3(void *pvParameters)
{
    uint8_t key = 0;                   // 10-1：局部变量存按键值

    while(1)
    {
        printf("task3正在运行\\r\\n");     // 10-2：串口打印

        key = key_scan(0);             // 10-3：扫描按键（参数 0 = 不支持连续按）

        if(key == KEY0_PRES)           // 10-4：检测 KEY0 是否被按下
        {
            if(task1_handle != NULL)   // 10-5：判断 task1 是否还存在
            {
                printf("删除task1任务\\r\\n");
                vTaskDelete(task1_handle);  // 10-6：删除 task1 任务
                task1_handle = NULL;        // 10-7：清空句柄，防止重复删除
            }
        }

        vTaskDelay(10);                // 10-8：延迟 10ms 再扫描
    }
}
```

**执行过程**：

12. `key = key_scan(0)` 读取按键状态，参数 `0` 表示不启用连按（按住也只触发一次）。
13. 如果返回值是 `KEY0_PRES`，说明 KEY0 被按下了。
14. 先检查 `task1_handle != NULL`，确认 `task1` 还没被删。
15. `vTaskDelete(task1_handle)` 删除 `task1`。此后 `task1` 消失，LED0 停止闪烁。
16. `task1_handle = NULL` 把句柄清空，防止下次再按 KEY0 时重复删除已死的任务。
17. `vTaskDelay(10)` 让任务阻塞 10ms。按键不需要扫描太快，10ms 足够。

---

## 整体运行时序图

```plain text
main() 调用 freertos_dome()
  │
  ├─→ xTaskCreate( start_task )        // 注册启动任务
  ├─→ vTaskStartScheduler()            // 启动调度器（永不返回）
  │
  └─→ 调度器开始工作 ──────────────────────────────┐
                                                    │
      start_task (优先级 1) 运行:                    │
      ├─→ 关中断                                     │
      ├─→ 创建 task1 (优先级 2)                       │
      ├─→ 创建 task2 (优先级 3)                       │
      ├─→ 创建 task3 (优先级 4)                       │
      ├─→ 开中断                                    │
      └─→ 自杀 (vTaskDelete)                        │
                                                    │
      调度器在 task1 / task2 / task3 之间切换:         │
                                                    │
      task3 (优先级 4) ◄── 最高优先级                  │
      │  每 10ms 扫描按键                            │
      │  按 KEY0 → 删除 task1                        │
      │  然后 vTaskDelay(10) → 阻塞                  │
      │                                             │
      task2 (优先级 3) ◄── 中等优先级                  │
      │  每 500ms 翻转 LED1 + 打印                    │
      │  然后 vTaskDelay(500) → 阻塞                 │
      │                                             │
      task1 (优先级 2) ◄── 最低优先级                  │
         每 500ms 翻转 LED0 + 打印                    │
         然后 vTaskDelay(500) → 阻塞                 │
                                                    │
      按下 KEY0 后:                                  │
      task1 被删除 → 只剩 task2 和 task3 在跑          │
```

---

## 优先级为什么这样设计

| 任务 | 优先级 | 理由 |
| --- | --- | --- |
| `task3`（按键） | **4（最高）** | 按键需要快速响应，10ms 扫一次，不能等 |
| `task2`（LED1） | 3 | 普通周期任务 |
| `task1`（LED0） | 2 | 普通周期任务 |
| `start_task` | 1（最低） | 只运行一次就自杀，不需要高优先级 |

优先级高的任务就绪时，低优先级的会被**抢占**。比如 `task1` 正在跑 `printf`，`task3` 的 `vTaskDelay(10)` 到期了，CPU 会立刻切给 `task3` 去扫描按键。

# 任务创建和删除(静态方法)

```javascript
// FreeRTOS 内核源码中，启动调度器时会这样调用你的函数：
vApplicationGetIdleTaskMemory(&tcb, &stack, &size);  // "给我空闲任务的内存"
// 然后它用你给的 tcb + stack 去创建空闲任务

vApplicationGetTimerTaskMemory(&tcb, &stack, &size); // "给我定时器任务的内存"
// 然后它用你给的 tcb + stack 去创建定时器任务
```

## `freertos_dome.c` 静态创建任务 — 逐段详解

---

### 第 1 段：头文件包含（第 1 ~ 7 行）

```c
#include "./SYSTEM/usart/usart.h"   // 正点原子串口驱动，printf 依赖它
#include "./BSP/LED/led.h"          // LED 驱动（LED0_TOGGLE / LED1_TOGGLE）
#include "./BSP/KEY/key.h"          // 按键驱动（key_scan / KEY0_PRES）
#include "freertos_dome.h"          // 本模块头文件

#include "FreeRTOS.h"               // FreeRTOS 核心头文件（StaticTask_t 在这定义）
#include "task.h"                   // 任务 API 头文件（xTaskCreateStatic / vTaskDelete 等）
```

**作用**：引入所有需要的外设驱动和 FreeRTOS API。

---

### 第 2 段：用户任务参数宏 + 静态内存声明（第 8 ~ 37 行）

```c
/* ========== start_task ========== */
#define START_TASK_PRIO         1               // 优先级：1（数字越小优先级越低）
#define START_TASK_STACK_SIZE   128             // 栈深度：128 字（Word = 4字节，即 512 字节）
TaskHandle_t start_task_handle;                 // 任务句柄（指针，用来操作任务）
StackType_t  start_task_stack[START_TASK_STACK_SIZE];  // ★ 静态栈：128 个 uint32_t 的数组
StaticTask_t start_tcb;                         // ★ 静态 TCB：任务控制块变量
void freertos_dome(void);                       // 入口函数声明

/* ========== task1 ========== */
#define TASK1_PRIO         2
#define TASK1_STACK_SIZE   128
TaskHandle_t task1_handle;
StackType_t  task1_stack[START_TASK_STACK_SIZE];
StaticTask_t task1_tcb;
void task1(void *pvParameters);

/* ========== task2 ========== */
#define TASK2_PRIO         3
#define TASK2_STACK_SIZE   128
TaskHandle_t task2_handle;
StackType_t  task2_stack[START_TASK_STACK_SIZE];
StaticTask_t task2_tcb;
void task2(void *pvParameters);

/* ========== task3 ========== */
#define TASK3_PRIO         4
#define TASK3_STACK_SIZE   128
TaskHandle_t task3_handle;
StackType_t  task3_stack[START_TASK_STACK_SIZE];
StaticTask_t task3_tcb;
void task3(void *pvParameters);
```

**每创建一个静态任务，必须准备 3 样东西**：

| 序号 | 声明 | 类型 | 含义 |
| --- | --- | --- | --- |
| ① | `xxx_handle` | `TaskHandle_t` | 任务句柄，保存任务引用，用于删除/挂起等操作 |
| ② | `xxx_stack[]` | `StackType_t` 数组 | 任务的专属栈（Run-time Stack），数组长度 = 栈深度 |
| ③ | `xxx_tcb` | `StaticTask_t` | 任务控制块（TCB），FreeRTOS 把任务状态都存在这里面 |

> 这里有个小细节：`task1_stack`、`task2_stack`、`task3_stack` 用的是 `START_TASK_STACK_SIZE` 而非各自的宏，效果一样（都是 128），但更规范的做法应该用各自定义的宏。

---

### 第 3 段：内核任务静态内存声明（第 39 ~ 45 行）

```c
/* 空闲任务配置 */
StaticTask_t    idel_task_tcb;                            // 空闲任务的 TCB
StackType_t     idel_task_stack[configMINIMAL_STACK_SIZE]; // 空闲任务的栈

/* 软件定时器任务配置 */
StaticTask_t    timer_task_tcb;                           // 定时器任务的 TCB
StackType_t     timer_task_stack[configMINIMAL_STACK_SIZE]; // 定时器任务的栈
```

**为什么要声明这两组？**

因为 `configSUPPORT_STATIC_ALLOCATION = 1` 告诉 FreeRTOS："不许用堆"。

FreeRTOS 启动调度器时会自动创建 空闲任务（必须）和 定时器任务（如果启用）。这两个任务不是你调 API 创建的，是内核自己建的。内核不知道内存在哪，所以你必须提前声明好 TCB + 栈，然后通过回调告诉它。

---

### 第 4 段：内核回调函数（第 48 ~ 65 行）

```c
/* 空闲任务内存分配 — 内核启动时自动调用 */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t  **ppxIdleTaskStackBuffer,
                                   uint32_t     *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &idel_task_tcb;       // 把空闲任务 TCB 的地址交给内核
    *ppxIdleTaskStackBuffer = idel_task_stack;       // 把空闲任务栈数组的地址交给内核
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE; // 告诉内核栈有多大
}

/* 软件定时器内存分配 — 内核启动时自动调用（如果启用了定时器） */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t  **ppxTimerTaskStackBuffer,
                                    uint32_t     *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer   = &timer_task_tcb;      // 把定时器任务 TCB 的地址交给内核
    *ppxTimerTaskStackBuffer = timer_task_stack;      // 把定时器任务栈数组的地址交给内核
    *pulTimerTaskStackSize   = configMINIMAL_STACK_SIZE; // 告诉内核栈有多大
}
```

**函数名拆解**：

```plain text
v          → void 返回类型
Application→ 应用程序层（你写的）
Get        → 获取
IdleTask   → 空闲任务
Memory     → 内存
```

**参数说明**（都是二级指针，内核用来接收你的地址）：

| 参数 | 类型 | 含义 |
| --- | --- | --- |
| `ppxIdleTaskTCBBuffer` | `StaticTask_t**` | 内核把你要的 TCB 指针写进这里 |
| `ppxIdleTaskStackBuffer` | `StackType_t**` | 内核把你要的栈指针写进这里 |
| `pulIdleTaskStackSize` | `uint32_t*` | 内核把栈大小写进这里 |

**整个流程**：内核启动 → 内核调用这两个函数（问你要内存）→ 你把预先分配好的 TCB 和栈地址"交"给内核 → 内核用这些内存创建空闲任务和定时器任务。

---

### 第 5 段：入口函数 `freertos_dome()`（第 67 ~ 77 行）

```c
void freertos_dome(void)
{
    start_task_handle = xTaskCreateStatic(
                (TaskFunction_t)    start_task,         // ① 任务函数指针
                (char *)            "start_task",       // ② 任务名字（调试用）
                (uint32_t)          START_TASK_STACK_SIZE, // ③ 栈深度 128
                (void *)            NULL,               // ④ 传递给任务的参数（无）
                (UBaseType_t)       START_TASK_PRIO,    // ⑤ 优先级 1
                (StackType_t *)     start_task_stack,   // ⑥ ★ 指向预分配的栈
                (StaticTask_t *)    &start_tcb);        // ⑦ ★ 指向预分配的 TCB
    vTaskStartScheduler();  // 启动调度器（之后由 FreeRTOS 接管）
}
```

`**xTaskCreateStatic**`** 7 个参数详解**：

| 参数 | 传入值 | 说明 |
| --- | --- | --- |
| ① `pxTaskCode` | `start_task` | 任务要执行的函数 |
| ② `pcName` | `"start_task"` | 任务名称，调试时在 IDE 的任务列表能看到 |
| ③ `ulStackDepth` | `128` | 栈深度，单位 Word（128 × 4 = 512 字节） |
| ④ `pvParameters` | `NULL` | 传给任务函数的参数，不需要就传 NULL |
| ⑤ `uxPriority` | `1` | 优先级，数字越大越高（0 保留给空闲任务） |
| ⑥ `**puxStackBuffer**` | `start_task_stack` | **静态分配独有的参数**，指向你预分配的栈 |
| ⑦ `**pxTaskBuffer**` | `&start_tcb` | **静态分配独有的参数**，指向你预分配的 TCB |

> 前 5 个参数和 `xTaskCreate` 一模一样，**只有最后两个是静态版本多出来的**。

**返回值**：`xTaskCreateStatic` 返回任务句柄，创建成功返回有效值，失败返回 `NULL`。

---

### 第 6 段：`start_task` 任务函数（第 79 ~ 107 行）

```c
void start_task(void *pvParameters)  // pvParameters 来自创建时的参数，此处为 NULL
{
    taskENTER_CRITICAL();   // 进入临界区，禁止任务切换，保证原子性

    /* ---- 创建 task1 ---- */
    task1_handle = xTaskCreateStatic(
                (TaskFunction_t)    task1,
                (char *)            "task1",
                (uint32_t)          TASK1_STACK_SIZE,
                (void *)            NULL,
                (UBaseType_t)       TASK1_PRIO,
                (StackType_t *)     task1_stack,      // ★ 预分配的栈
                (StaticTask_t *)    &task1_tcb);      // ★ 预分配的 TCB

    /* ---- 创建 task2 ---- */
    task2_handle = xTaskCreateStatic(
                (TaskFunction_t)    task2,
                (char *)            "task2",
                (uint32_t)          TASK2_STACK_SIZE,
                (void *)            NULL,
                (UBaseType_t)       TASK2_PRIO,
                (StackType_t *)     task2_stack,
                (StaticTask_t *)    &task2_tcb);

    /* ---- 创建 task3 ---- */
    task3_handle = xTaskCreateStatic(
                (TaskFunction_t)    task3,
                (char *)            "task3",
                (uint32_t)          TASK3_STACK_SIZE,
                (void *)            NULL,
                (UBaseType_t)       TASK3_PRIO,
                (StackType_t *)     task3_stack,
                (StaticTask_t *)    &task3_tcb);

    taskEXIT_CRITICAL();            // 退出临界区

    vTaskDelete(start_task_handle); // 删除自身，start_task 使命完成，不再需要
}
```

**为什么要用临界区？**

`taskENTER_CRITICAL()` 会暂时关中断/禁止调度，确保三个任务一次性创建完毕，不被其他任务打断。三个任务都建好后 `taskEXIT_CRITICAL()` 恢复。如果不加，可能建到一半被抢占，导致不一致。

**为什么要删除自己？**

`start_task` 的唯一职责就是"创建 task1、task2、task3 并启动它们"。这件事做完后它就没用了，继续存在只会浪费 CPU 和内存，所以 `vTaskDelete(start_task_handle)` 把自己删掉。

---

### 第 7 段：`task1` 任务函数（第 109 ~ 117 行）

```c
void task1(void *pvParameters)
{
    while(1)                    // 任务函数必须是无尽循环
    {
        printf("task1正在运行\\r\\n");   // 串口打印
        LED0_TOGGLE();          // 翻转 LED0 状态（亮→灭→亮→灭...）
        vTaskDelay(500);        // 延时 500 个 tick（阻塞态，让出 CPU）
    }
}
```

| 步骤 | 做了什么 |
| --- | --- |
| `while(1)` | 任务必须是死循环，因为任务函数永远不能 return |
| `printf` | 通过串口打印运行信息 |
| `LED0_TOGGLE()` | 翻转 LED0 |
| `vTaskDelay(500)` | **关键**：任务进入阻塞态 500ms，CPU 去运行其他任务 |

---

### 第 8 段：`task2` 任务函数（第 119 ~ 127 行）

```c
void task2(void *pvParameters)
{
    while(1)
    {
        printf("task2正在运行\\r\\n");
        LED1_TOGGLE();
        vTaskDelay(500);
    }
}
```

和 task1 结构一样，操作的是 LED1。两个任务交替闪烁两个 LED。

---

### 第 9 段：`task3` 任务函数（第 129 ~ 147 行）

```c
void task3(void *pvParameters)
{
    uint8_t key = 0;
    while(1)
    {
        printf("task3正在运行\\r\\n");
        key = key_scan(0);              // 扫描按键（0 = 不支持连按）
        if(key == KEY0_PRES)            // 如果 KEY0 按下
        {
            if(task1_handle != NULL)    // 确保 task1 还活着
            {
                printf("删除task1任务\\r\\n");
                vTaskDelete(task1_handle);  // ★ 删除 task1
                task1_handle = NULL;        // 句柄置空，防止重复删除
            }
        }
        vTaskDelay(10);                 // 10ms 扫描一次
    }
}
```

**作用**：task3 监控按键 KEY0，按下去就删除 task1（LED0 停止闪烁），再按不会再删（句柄已置 NULL）。

**为什么要 **`**task1_handle = NULL**`**？**

`vTaskDelete` 只是释放任务资源，但不会修改句柄的值。如果不置 NULL，下次按键时 `task1_handle != NULL` 仍然为真，会再一次 `vTaskDelete(task1_handle)` —— 删除一个已经不存在的任务，后果未定义（可能死机）。

---

## 完整运行流程总结

```plain text
main() 调用 freertos_dome()
  │
  ├─ ① xTaskCreateStatic 创建 start_task（传入预分配的栈 + TCB）
  │
  ├─ ② vTaskStartScheduler() 启动调度器
  │       │
  │       ├─ 内核自动调用 vApplicationGetIdleTaskMemory() 拿空闲任务内存
  │       ├─ 内核自动调用 vApplicationGetTimerTaskMemory() 拿定时器任务内存
  │       ├─ 内核创建空闲任务 + 定时器任务
  │       └─ 调度器开始运转，选出最高优先级就绪任务运行
  │
  └─ ③ start_task 获得 CPU
        │
        ├─ taskENTER_CRITICAL() 禁止打断
        ├─ xTaskCreateStatic 创建 task1 (优先级2, LED0)
        ├─ xTaskCreateStatic 创建 task2 (优先级3, LED1)
        ├─ xTaskCreateStatic 创建 task3 (优先级4, 按键监控)
        ├─ taskEXIT_CRITICAL() 恢复打断
        └─ vTaskDelete(start_task_handle) 删除自己
              │
              ▼
         task3 优先级最高(4)，先运行
           → 打印 "task3正在运行"，扫描按键，延时 10ms
              │
              ▼
         task2 优先级(3)，运行
           → 打印 "task2正在运行"，翻转 LED1，延时 500ms（阻塞）
              │
              ▼
         task1 优先级(2)，运行
           → 打印 "task1正在运行"，翻转 LED0，延时 500ms（阻塞）
              │
              ▼
         空闲任务(优先级0) — 所有任务阻塞时运行
              │
              ▼
         （循环往复，task3 每隔 10ms 检查按键，
           task1/task2 每隔 500ms 切换 LED 状态）
```

---

## 和动态创建的核心差别速查

|   | 动态版本 | 静态版本 |
| --- | --- | --- |
| 创建 API | `xTaskCreate()` | `xTaskCreateStatic()` |
| 栈来源 | 自动 `pvPortMalloc` | 你写的 `StackType_t xxx_stack[128]` |
| TCB 来源 | 自动 `pvPortMalloc` | 你写的 `StaticTask_t xxx_tcb` |
| 内核任务内存 | 不需要管 | 必须写 `vApplicationGetIdleTaskMemory` 和 `vApplicationGetTimerTaskMemory` |
| FreeRTOSConfig.h | 无特殊要求 | 必须 `#define configSUPPORT_STATIC_ALLOCATION 1` |
| 编译后 RAM 占用 | 不确定（堆分配） | 确定（全局变量，编译时就定了） |
