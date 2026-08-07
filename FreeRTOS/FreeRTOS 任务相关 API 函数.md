---
notion-id: 3855d378-8ef5-80c4-af8e-c717f652a9fa
---
## **一、FreeRTOS 任务相关 API 函数介绍**

### **1.1 任务优先级相关**

### `**uxTaskPriorityGet()**`** — 获取任务优先级**

```plain text
UBaseType_t uxTaskPriorityGet(const TaskHandle_t xTask);
```

- **形参** `xTask`：要查询的任务句柄。传入 `NULL` 表示查询当前任务
- **返回值**：任务优先级
- **前提宏**：`INCLUDE_uxTaskPriorityGet` 置 1

---

### `**vTaskPrioritySet()**`** — 设置任务优先级**

```plain text

void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority);
```

- **形参**：
    - `xTask`：要修改的任务句柄，`NULL` 表示当前任务
    - `uxNewPriority`：新的优先级
- **前提宏**：`INCLUDE_vTaskPrioritySet` 置 1

---

### **1.2 任务数量相关**

### `**uxTaskGetNumberOfTasks()**`** — 获取任务数量**

```plain text

UBaseType_t uxTaskGetNumberOfTasks(void);
```

- **返回值**：系统中当前的任务总数

---

### **1.3 任务状态 / 信息查询相关**

### `**uxTaskGetSystemState()**`** — 获取所有任务的状态信息**

```plain text

UBaseType_t uxTaskGetSystemState(
    TaskStatus_t * const            pxTaskStatusArray,
    const UBaseType_t               uxArraySize,
    configRUN_TIME_COUNTER_TYPE *   const pulTotalRunTime
);
```

- **形参**：
    - `pxTaskStatusArray`：`TaskStatus_t` 数组，用于存放各任务状态信息
    - `uxArraySize`：数组容量
    - `pulTotalRunTime`：输出系统总运行时间
- **前提宏**：`configUSE_TRACE_FACILITY` 置 1

`**TaskStatus_t**`** 结构体：**

```plain text

typedef struct xTASK_STATUS
{
    TaskHandle_t                xHandle;              /* 任务句柄 */
    const char *                pcTaskName;           /* 任务名 */
    UBaseType_t                 xTaskNumber;          /* 任务编号 */
    eTaskState                  eCurrentState;        /* 任务状态 */
    UBaseType_t                 uxCurrentPriority;    /* 任务优先级 */
    UBaseType_t                 uxBasePriority;       /* 任务原始优先级 */
    configRUN_TIME_COUNTER_TYPE ulRunTimeCounter;     /* 任务运行时间 */
    StackType_t *               pxStackBase;          /* 任务栈基地址 */
    configSTACK_DEPTH_TYPE      usStackHighWaterMark;  /* 任务栈历史剩余最小值 */
} TaskStatus_t;
```

---

### `**vTaskGetInfo()**`** — 获取单个任务的状态信息**

```plain text

void vTaskGetInfo(
    TaskHandle_t    xTask,
    TaskStatus_t *  pxTaskStatus,
    BaseType_t      xGetFreeStackSpace,
    eTaskState      eState
);
```

- **形参**：
    - `xTask`：要查询的任务句柄
    - `pxTaskStatus`：用于存放任务状态信息的结构体指针
    - `xGetFreeStackSpace`：是否计算剩余栈空间
    - `eState`：期望的状态（过滤条件）
- **前提宏**：`configUSE_TRACE_FACILITY` 置 1

`**eTaskState**`** 枚举（任务状态）：**

```plain text

typedef enum
{
    eRunning = 0,   /* 运行态 */
    eReady,         /* 就绪态 */
    eBlocked,       /* 阻塞态 */
    eSuspended,     /* 挂起态 */
    eDeleted,       /* 任务被删除 */
    eInvalid        /* 无效 */
} eTaskState;
```

---

### `**eTaskGetState()**`** — 查询某个任务的运行状态**

```plain text

eTaskState eTaskGetState(TaskHandle_t xTask);
```

- **形参** `xTask`：要查询的任务句柄
- **返回值**：当前 `eTaskState` 枚举状态
- **前提宏**：`INCLUDE_eTaskGetState` 置 1

---

### `**vTaskList()**`** — 表格形式列出所有任务信息**

```plain text

void vTaskList(char * pcWriteBuffer);
```

- **形参** `pcWriteBuffer`：用于存放表格字符串的缓冲区
- **前提宏**：
    - `configUSE_TRACE_FACILITY` 置 1
    - `configUSE_STATS_FORMATTING_FUNCTIONS` 置 1

**输出表格字段说明：**

| **字段** | **含义** |
| --- | --- |
| **Name** | 创建任务时分配的任务名 |
| **State** | 任务的壮态信息：`B` = 阻塞态，`R` = 就绪态，`S` = 挂起态，`D` = 删除态 |
| **Priority** | 任务优先级 |
| **Stack** | 任务堆栈"高水位线"，即堆栈历史最小剩余大小 |
| **Num** | 任务编号（唯一，同名的多个任务可通过此编号区分） |

---

### **1.4 任务句柄相关**

### `**xTaskGetCurrentTaskHandle()**`** — 获取当前任务句柄**

```plain text

TaskHandle_t xTaskGetCurrentTaskHandle(void);
```

- **返回值**：当前正在运行的任务句柄
- **前提宏**：`INCLUDE_xTaskGetCurrentTaskHandle` 置 1

---

### `**xTaskGetHandle()**`** — 通过任务名获取任务句柄**

```plain text

TaskHandle_t xTaskGetHandle(const char * pcNameToQuery);
```

- **形参** `pcNameToQuery`：任务名（字符串）
- **返回值**：对应任务的句柄
- **前提宏**：`INCLUDE_xTaskGetHandle` 置 1

---

### **1.5 任务栈相关**

### `**uxTaskGetStackHighWaterMark()**`** — 获取任务栈历史最小剩余**

```plain text

UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t xTask);
```

- **形参** `xTask`：要查询的任务句柄，`NULL` 表示当前任务
- **返回值**：历史最小剩余栈空间（以 word 为单位）
- **前提宏**：`INCLUDE_uxTaskGetStackHighWaterMark` 置 1

---

## **二、任务时间统计相关 API 函数**

### **2.1 **`**vTaskGetRunTimeStats()**`** — 统计任务运行时间**

```plain text

void vTaskGetRunTimeStats(char * pcWriteBuffer);
```

- **形参** `pcWriteBuffer`：用于存放统计表格字符串的缓冲区
- **前提宏**：
    - `configGENERATE_RUN_TIME_STATS` 置 1
    - `configUSE_STATS_FORMATTING_FUNCTIONS` 置 1

**输出表格字段说明：**

| **字段** | **含义** |
| --- | --- |
| **Task** | 任务名称 |
| **Abs Time** | 任务实际运行的总时间（绝对时间） |
| **% Time** | 占总处理时间的百分比 |

---

### **2.2 使用流程**

1. 将宏 `configGENERATE_RUN_TIME_STATS` 置 1
2. 将宏 `configUSE_STATS_FORMATTING_FUNCTIONS` 置 1
3. 当 `configGENERATE_RUN_TIME_STATS` 置 1 后，还需要实现 **2 个宏定义**：
    - `**portCONFIGURE_TIMER_FOR_RUNTIME_STATE()**`：用于初始化配置任务运行时间统计的时基定时器 ⚠️ 注意：这个时基定时器的计时精度需高于系统时钟节拍精度的 **10 至 100 倍**！
    - `**portGET_RUN_TIME_COUNTER_VALUE()**`：用于获取该时基硬件定时器的计数值

---

## **三、实验设计**

### **实验1：任务状态查询 API 实验（掌握）**

**实验目的：** 学习 FreeRTOS 任务状态与信息的查询 API 函数

**实验设计：** 三个任务 —— `start_task`、`task1`、`task2`

| **任务** | **功能** |
| --- | --- |
| `start_task` | 用于创建 `task1` 和 `task2` 任务 |
| `task1` | LED0 每 500ms 闪烁一次，提示程序正在运行 |
| `task2` | 展示任务状态信息查询相关 API 函数的使用 |

---

### **实验2：任务时间统计 API 实验（掌握）**

**实验目的：** 学习 FreeRTOS 任务运行时间统计相关 API 函数的使用

**实验设计：** 三个任务 —— `start_task`、`task1`、`task2`

| **任务** | **功能** |
| --- | --- |
| `start_task` | 用于创建 `task1` 和 `task2` 任务 |
| `task1` | LED0 每 500ms 闪烁一次，提示程序正在运行 |
| `task2` | 展示任务运行时间统计相关 API 函数的使用 |

---

## **四、课堂总结**

查看《FreeRTOS 任务相关 API 函数介绍》脑图。

---

## **API 函数速查表**

| **函数** | **功能** | **前提宏** |
| --- | --- | --- |
| `uxTaskPriorityGet(xTask)` | 获取任务优先级 | `INCLUDE_uxTaskPriorityGet` |
| `vTaskPrioritySet(xTask, prio)` | 设置任务优先级 | `INCLUDE_vTaskPrioritySet` |
| `uxTaskGetNumberOfTasks()` | 获取系统任务总数 | — |
| `uxTaskGetSystemState(...)` | 获取所有任务的状态信息 | `configUSE_TRACE_FACILITY` |
| `vTaskGetInfo(...)` | 获取单个任务的状态信息 | `configUSE_TRACE_FACILITY` |
| `eTaskGetState(xTask)` | 查询任务运行状态 | `INCLUDE_eTaskGetState` |
| `vTaskList(buf)` | 表格形式列出任务信息 | `configUSE_TRACE_FACILITY` + `configUSE_STATS_FORMATTING_FUNCTIONS` |
| `xTaskGetCurrentTaskHandle()` | 获取当前任务句柄 | `INCLUDE_xTaskGetCurrentTaskHandle` |
| `xTaskGetHandle(name)` | 通过任务名获取句柄 | `INCLUDE_xTaskGetHandle` |
| `uxTaskGetStackHighWaterMark(xTask)` | 获取任务栈高水位线 | `INCLUDE_uxTaskGetStackHighWaterMark` |
| `vTaskGetRunTimeStats(buf)` | 统计任务运行时间 | `configGENERATE_RUN_TIME_STATS` + `configUSE_STATS_FORMATTING_FUNCTIONS` |