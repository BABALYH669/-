# FreeRTOS 简答题 · 查缺补漏自测卷（题目 + 参考答案）

> 位置：`D:\Obsidian\Obsidian\FreeRTOS\`（20 篇笔记）
> 用法：先默答 → 再对照答案 → 打勾/画叉，标记不熟的就是薄弱点。
> 风格：与你 FreeRTOS 笔记一致，含对比表与例程细节（正点原子 F103 例程）。

---

## 一、裸机 vs RTOS 与 FreeRTOS 基础

### 1. 裸机与 RTOS 在"任务组织、延时、实时性"上有什么本质区别？

| 对比 | 裸机（前后台） | RTOS |
| --- | --- | --- |
| 结构 | 所有功能堆在一个 `while(1)` 大循环（后台）+ 中断（前台） | 功能拆成多个独立任务，各管各的 |
| 执行 | 顺序执行，前面没跑完后面只能等 | 调度器按时间片/优先级切换，宏观像"同时运行" |
| 延时 | `delay` 空等待，CPU 白耗 | 系统延时 = 任务调度，延时期间 CPU 去跑别的任务 |
| 实时性 | 差 | 抢占式：高优先级就绪立即抢占 |

一句话：**裸机是"排队干活"，RTOS 是"按优先级插队 + 时间片轮转"**。

### 2. FreeRTOS 支持哪三种调度方式？各自针对什么场景？

1. **抢占式调度**：针对**优先级不同**的任务——高优先级任务就绪时，立刻抢占正在运行的低优先级任务（`configUSE_PREEMPTION=1`）。
2. **时间片调度**：针对**优先级相同**的任务——同优先级任务在**每一次系统时钟节拍（tick）到来时**轮换执行（`configUSE_TIME_SLICING=1`）。
3. **协程式调度**：当前任务一直运行、不抢占；任务必须主动让出 CPU（已基本不用）。

### 3. FreeRTOS 任务有哪四种状态？各自怎么进入、怎么离开？

| 状态 | 含义 | 进入方式 | 离开方式 |
| --- | --- | --- | --- |
| 运行态 | 正在占用 CPU 的任务（单核同一时刻只有一个） | 被调度器选中 | 被抢占/阻塞/挂起 |
| 就绪态 | 已能运行但还没轮到 | 延时到期/被恢复 | 被调度器选中运行 |
| 阻塞态 | 因延时或等待事件/资源而暂停 | `vTaskDelay()`、等待队列/信号量 | 延时到/事件到 |
| 挂起态 | 类似"暂停"，不参与调度 | `vTaskSuspend()` | `vTaskResume()`/`xTaskResumeFromISR()` |

### 4. FreeRTOS 的优先级规则和任务数量有什么特点？

- **数字越大优先级越高**（0 最低，通常留给空闲任务）；与 NVIC 中断优先级"数字越小越优先"**正好相反**。
- 优先级分配无软件限制（实际受 MCU/`configMAX_PRIORITIES` 约束，例程 = 32，即 0~31）。
- 支持**多个任务同一优先级**（靠时间片调度）。
- 任务创建数量无软件限制，仅受 RAM 约束；每个任务有**独立栈空间**，切换后从断点恢复（`pxTopOfStack` 记录栈顶，是任务切换的关键）。

---

## 二、任务创建与删除

### 1. 动态创建（`xTaskCreate`）与静态创建（`xTaskCreateStatic`）的区别？

| 对比 | 动态 `xTaskCreate` | 静态 `xTaskCreateStatic` |
| --- | --- | --- |
| TCB 来源 | FreeRTOS 从管理的堆中自动 `pvPortMalloc` | 用户提供的 `StaticTask_t xxx_tcb` |
| 栈来源 | 堆中自动分配 | 用户提供的 `StackType_t xxx_stack[128]` |
| 前提宏 | `configSUPPORT_DYNAMIC_ALLOCATION=1` | `configSUPPORT_STATIC_ALLOCATION=1` |
| 内核任务内存 | 不用管 | 必须实现 `vApplicationGetIdleTaskMemory` / `vApplicationGetTimerTaskMemory` 回调，把空闲任务、定时器任务的 TCB+栈地址交给内核 |
| 参数 | 6 个 | 前 5 个相同，**多最后 2 个**：`puxStackBuffer`（栈）、`pxTaskBuffer`（TCB） |
| 内存占用 | 运行时堆分配，不确定 | 编译期就确定的全局变量 |

**为什么静态方式要给空闲任务和定时器任务也准备内存**：这两个任务不是用户调 API 创建的，而是 `vTaskStartScheduler()` 时内核**自己创建**的，内核不知道内存从哪来，只能通过你实现的回调问你要。

### 2. `vTaskDelete(NULL)` 与 `vTaskDelete(句柄)` 在"内存释放时机"上有什么不同？

- 删除**别的任务**（传句柄）：在 `vTaskDelete` **调用处立即释放**被删任务的 TCB 与栈内存；
- 删除**自己**（传 `NULL`）：释放内存的工作要**交给空闲任务**完成——因为当前任务还站在自己的栈上执行删除操作，不能在调用处立刻释放自己的栈；
- **静态方式创建的任务**：内存是用户提供的，删除后必须**用户自己手动释放/回收**（内核不会帮你）。

### 3. 为什么例程里要先 `vTaskDelete(task1_handle)`，再 `task1_handle = NULL`？

`vTaskDelete` 只负责释放任务资源，**不会修改句柄的值**。若句柄不清空，下次按键时 `task1_handle != NULL` 依然为真，会再次对一个已删除的任务执行删除——**后果未定义（可能死机）**。置 `NULL` 后再配合 `if(handle != NULL)` 判断，实现"只删一次"。

### 4. 为什么在 `start_task` 里创建多个子任务时要包在 `taskENTER_CRITICAL()/taskEXIT_CRITICAL()` 之间？创建完为什么还要"自杀"？

- 临界区 = **关中断/禁止任务切换**，保证 3 个任务"一次性原子创建"，不会被抢占到一半导致状态不一致。
- "自杀"（`vTaskDelete(NULL)`）：`start_task` 的唯一职责是创建子任务，干完活没必要继续占用 CPU 和内存，删掉节省资源。注意 `vTaskDelete(NULL)` **不会返回**，调用后该任务就消失了。

### 5. 任务函数为什么必须是 `while(1)` 死循环？任务函数能不能 `return`？

不能 `return`。任务函数返回相当于任务"结束但没有被删除"，FreeRTOS 会触发断言/进入错误处理（行为未定义，通常崩溃）。任务要么永远循环，要么在退出前显式调用 `vTaskDelete(NULL)` 删除自己。

### 6. `xTaskCreate` 的形参有哪 6 个？栈大小单位是什么？

1. 任务函数指针 `pxTaskCode`
2. 任务名 `pcName`（调试用，最大 `configMAX_TASK_NAME_LEN=16` 字符）
3. 栈深度 `usStackDepth`（单位是 **word，即 4 字节**；128 = 512 字节）
4. 传给任务的参数 `pvParameters`（无则 `NULL`）
5. 优先级 `uxPriority`（数字大优先高）
6. 任务句柄地址 `&pxCreatedTask`（`TaskHandle_t`，"遥控器"，后续靠它挂起/恢复/删除）

---

## 三、调度器启动与任务切换机制

### 1. `vTaskStartScheduler()` 内部做了哪些事？

1. 创建**空闲任务**（若使能软件定时器则同时创建**定时器服务任务**）；
2. **关闭中断**（防止调度器开启前/过程中被中断干扰，运行第一个任务时会再打开）；
3. 初始化全局变量、把调度器运行标志置为"已运行"；
4. 初始化任务运行时间统计的时基定时器（若使能）；
5. 调用 `xPortStartScheduler()`。

### 2. `xPortStartScheduler()` 负责什么？

与硬件架构相关的启动配置：

1. 检测 `FreeRTOSConfig.h` 中断配置是否有误；
2. 配置 **PendSV 和 SysTick 为最低优先级**；
3. 配置 SysTick 定时器（`vPortSetupTimerInterrupt`）；
4. 初始化临界区嵌套计数器为 0；
5. 使能 FPU；
6. 调用 `prvStartFirstTask()` 启动第一个任务。

### 3. SVC 和 PendSV 在 FreeRTOS 中各起什么作用？为什么 PendSV 要设成最低优先级？

- **SVC**（系统服务调用）：在 `vTaskStartScheduler` 中触发，用来**启动第一个任务**（从"内核态"进入第一个任务的上下文）；
- **PendSV**（可挂起的系统调用）：用于**执行任务上下文切换**。把它设为**最低优先级**，是为了让切换**推迟到所有其它中断处理完毕之后**进行——中断正在服务时不会被打断去做任务切换，保证中断响应的实时性。

（配置文件里通过 `xPortPendSVHandler → PendSV_Handler`、`vPortSVCHandler → SVC_Handler` 把内核函数映射到启动文件的向量表。）

---

## 四、时间片、系统节拍与延时管理

### 1. 什么是时间片？FreeRTOS 中一个时间片等于什么？

时间片 = 同优先级任务**轮流享有的 CPU 时间**。在 FreeRTOS 中**一个时间片 = 一个 SysTick 中断周期**（例程 `configTICK_RATE_HZ=1000` 时 = 1ms，同优先级任务每 1ms 轮换一次）。

### 2. `configTICK_RATE_HZ` 是什么？`vTaskDelay(1)` 到底延时多久？

`configTICK_RATE_HZ` = 每秒的 tick 次数（Hz），**每个 tick 的时间 = 1/configTICK_RATE_HZ**。

- `configTICK_RATE_HZ = 1000` → 1 tick = 1ms → `vTaskDelay(1)` = 1ms；
- 若配置成 20Hz → 1 tick = 50ms → `vTaskDelay(1)` = **50ms**，所有延时都以 50ms 为最小粒度。

**权衡**：频率高精度好但 SysTick 中断开销大；频率低（如 20Hz）适合低功耗、对实时性要求不高的场景（每秒中断从 1000 次降到 20 次）。

### 3. `pdMS_TO_TICKS(ms)` 在低频 tick 下有什么坑？

该宏按 `ms / tick周期` 换算并**向下取整**。例程 tick=50ms 时：

- `pdMS_TO_TICKS(30)` → 30/50 = **0 个 tick**（30ms 延时直接被截断成 0，任务不阻塞，这是个隐蔽 bug）；
- `pdMS_TO_TICKS(50)` → 1 tick；`pdMS_TO_TICKS(80)` → 也是 1 tick（精度损失）。

要支持 10ms 级延时就得把 `configTICK_RATE_HZ` 提到 100Hz（10ms/tick）以上。

### 4. 相对延时 `vTaskDelay()` 与绝对延时 `vTaskDelayUntil()` 的区别？分别适合什么场景？

| | 相对延时 `vTaskDelay` | 绝对延时 `vTaskDelayUntil` |
| --- | --- | --- |
| 计时起点 | 每次从**调用该函数那一刻**开始计时 | 以**固定的周期起点**为基准（把任务运行周期看成一个整体） |
| 实际周期 | 任务执行时间 + 延时时间，**周期不固定**（受高优先级抢占影响，波形抖动） | **固定周期**：任务以恒定频率执行 |
| 适用 | 普通延时、让出 CPU | 需要**按固定频率/周期运行**的任务（采样、刷新显示等） |

注意：绝对延时要求**任务主体运行时间必须小于延时周期**，否则会错过周期起点、失去"绝对"意义。低优先级任务用相对延时测出的波形不固定，正是因为会被高优先级抢占。

### 5. `vTaskDelay()` 内部（软件层面）是怎么让任务"睡过去"又"醒过来"的？

1. 判断延时 > 0；
2. **挂起调度器**；
3. 把当前任务从就绪列表移除，加入**阻塞（延时）列表**（记录阻塞超时时间存进列表项值，用来按升序确定插入位置；若唤醒时刻溢出 tick 计数器则加入**溢出阻塞列表**）；
4. 恢复调度器；
5. 触发一次任务切换。

之后由 **SysTick（滴答）中断计时**：每个 tick 检查阻塞列表头部，到时间的任务从阻塞列表移除、插回就绪列表等待调度。

---

## 五、列表与列表项（调度器的数据结构基础）

### 1. FreeRTOS 为什么要用"列表（链表）"而不是数组来管理任务？

- 列表 = 双向环形链表，列表项间地址**非连续**、靠指针人为连接，**数目可随时增删**；数组地址连续、成员数量定义后就固定。
- OS 中任务数量不确定、任务状态（就绪/阻塞/挂起）随时变化，需要频繁插入/删除，所以链表这种动态结构最合适。

### 2. `List_t`、`ListItem_t`、`MiniListItem_t` 各自的关键成员和作用？

| 结构 | 关键成员 | 作用 |
| --- | --- | --- |
| `List_t` | `uxNumberOfItems`（列表项个数，不含哨兵）、`pxIndex`（遍历游标）、`xListEnd` | 列表头 |
| `ListItem_t` | `xItemValue`（排序值）、`pxNext`/`pxPrevious`、`pvOwner`（指向拥有者，通常是 TCB）、`pxContainer`（指向所在列表） | 正常列表项 |
| `MiniListItem_t` | 只有 `xItemValue` + 前后指针 | **哨兵/终点标记**：值固定为 `portMAX_DELAY`（0xFFFFFFFF），永远在末尾；没有 owner/container，省内存 |

**空列表 = 哨兵 xListEnd 自己指向自己（自环）**，`uxNumberOfItems=0`。

### 3. `vListInsert()` 与 `vListInsertEnd()` 有什么区别？各用在哪？

| | `vListInsert` | `vListInsertEnd` |
| --- | --- | --- |
| 插入位置 | 按 `xItemValue` **升序**自动找位置（值小在前；最大值的插在 xListEnd 前） | **插在 pxIndex 指向的列表项之前**（不管值大小） |
| 典型用途 | 按优先级/超时时间排序 → 就绪列表、延时列表 | 按时间顺序（FIFO 感）→ 时间片轮转、同优先级轮流 |

`uxListRemove()` 把列表项从链表中"摘掉"：前后邻居直接相连跳过自己，并把 `pxContainer` 置 NULL（标记"我不在任何列表中"）。

### 4. FreeRTOS 调度器本质是什么？任务一生要在哪些列表间"搬家"？

**调度器本质 = 一堆环形双向链表，把任务 TCB 在不同列表中移来移去**：

```
创建 → 就绪列表（按优先级分桶，pxReadyTasksLists[0~31]）
vTaskDelay → 从就绪列表搬到延时列表（xDelayedTaskList，按唤醒时间升序）
SysTick 到期 → 从延时列表搬回就绪列表
vTaskSuspend → 搬到挂起列表（xSuspendedTaskList）
vTaskResume → 搬回就绪列表
vTaskDelete → 从列表中移除并释放
```

- 就绪列表：**每个优先级一个列表**，调度器从最高优先级的非空列表取任务；
- 延时列表分两个（`xDelayedTaskList1/2`）：tick 计数器 32 位会溢出，用两个列表交替处理"未溢出/已溢出"的唤醒时刻；
- SysTick 每个 tick 扫描延时列表头部（值最小 = 最早该醒的任务）。

---

## 六、任务挂起与恢复

### 1. "挂起"与"删除"有什么区别？

| | 挂起 `vTaskSuspend` | 删除 `vTaskDelete` |
| --- | --- | --- |
| 结果 | 类似**暂停**：任务离开调度但不销毁 | **无法恢复**：TCB+栈被清除释放 |
| 恢复 | `vTaskResume`/`xTaskResumeFromISR` 恢复（挂起列表→就绪列表） | 不可恢复，只能重新创建 |

挂起后任务**保留自己的上下文/栈**，恢复时从"断点"接着跑；删除后一切资源回收。

### 2. 任务上下文调用 `vTaskResume()`，中断上下文调用 `xTaskResumeFromISR()`——两者区别？

| | 任务上下文 | 中断上下文 |
| --- | --- | --- |
| 恢复函数 | `vTaskResume(handle)` | `xTaskResumeFromISR(handle)` |
| 返回值 | 无 | `pdTRUE`（恢复出更高优先级任务，需立即切换）/ `pdFALSE` |

**中断里不能用普通版**：普通版内部可能触发阻塞/调度动作，中断上下文不能阻塞；FromISR 版只做"标记 + 把任务从挂起列表移回就绪列表"，是否立即切换由返回值告诉调用者。

### 3. 在中断里恢复任务后，为什么要检查返回值并调用 `portYIELD_FROM_ISR()`？

`xTaskResumeFromISR` 返回 `pdTRUE` 说明"被恢复的任务优先级比当前被打断的任务更高"，此时应**立即切换**而不是等下一个 tick。例程写法：

```c
xYieldRequired = xTaskResumeFromISR(task1_handle);
...
if(xYieldRequired == pdTRUE) { portYIELD_FROM_ISR(xYieldRequired); }  // 触发 PendSV，中断退出后立刻切走
```

若恢复的任务优先级不高于当前任务，则返回 `pdFALSE`，中断退出后继续执行原任务即可。

### 4. 为什么例程要把任务句柄声明成全局变量，并用 `extern` 跨文件使用？

`task1_handle` 等定义在 `freertos_dome.c`，而按键任务（`task3`）和中断回调（`exti.c`）都要用它来挂起/恢复对应任务。`extern TaskHandle_t task1_handle;` 告诉编译器"这个变量在别的 C 文件里定义"，从而实现跨文件访问。挂起/恢复/删除前都先判 `handle != NULL`，防止操作已不存在的任务。

---

## 七、中断管理与临界段保护

### 1. STM32 的 NVIC 中断优先级是如何分组的？FreeRTOS 为什么建议"全部用作抢占优先级"？

- Cortex-M3 用 8 位优先级寄存器，STM32 **只用高 4 位 [7:4]** → 最多 **16 级**（0~15），数值越小越优先；分抢占优先级和子优先级两组，`HAL_NVIC_SetPriorityGrouping` 有 0~4 五种分组（如 Group_0：全做子优先级；Group_4：全做抢占）。
- FreeRTOS 建议**把所有位都用作抢占优先级（Group_4）**：FreeRTOS 用 `BASEPRI` 做"阈值屏蔽"，只认一个线性优先级数值才方便管理；如果抢占/子优先级还要二次比较，阈值判断会失效。

### 2. FreeRTOS 中 `configMAX_SYSCALL_INTERRUPT_PRIORITY = 5` 是什么意思？

把中断优先级分成两区（**0 最高、15 最低**）：

| 中断优先级 | 是否被 FreeRTOS 管理 | 能否调用 FreeRTOS API |
| --- | --- | --- |
| 0 ~ 4（高优先级） | **管不到**（硬实时区） | **禁止**调用任何 FreeRTOS API（会破坏内核数据结构），ISR 要极短 |
| 5 ~ 15（低优先级） | 可管理（可被屏蔽） | 可调用，但必须用 **带 `FromISR` 后缀** 的函数（`xSemaphoreGiveFromISR`、`xQueueSendFromISR` 等） |

寄存器换算：`configMAX_SYSCALL_INTERRUPT_PRIORITY = 5 << 4 = 0x50`；`configKERNEL_INTERRUPT_PRIORITY = 15 << 4 = 0xF0`（SysTick/PendSV 用最低优先级）。例程 TIM7 抢占优先级 4（管不到）、TIM6 为 6（管得到），`portDISABLE_INTERRUPTS()` 关 5 秒时只有 TIM6 消失、TIM7 照常中断。

### 3. `portDISABLE_INTERRUPTS()` / `portENABLE_INTERRUPTS()` 到底关的是什么？

不是关所有中断，而是通过写 **BASEPRI = 0x50** 屏蔽"优先级 5~15"的中断（`BASEPRI`：屏蔽优先级低于某一阈值的中断）；**优先级 0~4 的中断不受影响**，照常可打断当前代码。恢复时 `portENABLE_INTERRUPTS()` 把 BASEPRI 写 0。例程现象：关中断期间 TIM6（优先级 6）不再打印，TIM7（优先级 4）继续运行。

### 4. 什么是临界区？`taskENTER_CRITICAL()` 与 `vTaskSuspendAll()` 有何不同？

**临界区（临界段）**：必须完整运行、不能被打断的代码段，本质就是**关中断/开中断**（任务调度和 ISR 都依赖中断，屏蔽中断就没人能打断临界代码）。适用：外设按严格时序初始化（IIC/SPI）、系统自身需求、用户共享资源访问。

| | `taskENTER_CRITICAL()/taskEXIT_CRITICAL()` | `vTaskSuspendAll()/xTaskResumeAll()` |
| --- | --- | --- |
| 是否关中断 | **关中断**（任务级）；另有 `..._FROM_ISR()` 版本 | **不关中断**，只挂起任务调度器 |
| 屏蔽范围 | 中断 + 任务切换全被挡住 | 中断**照常响应**，只防止任务间资源争夺/切换 |
| 适用 | 临界代码极短；要挡住一切 | 临界区在任务与任务之间，既要防调度又不想延时中断 |

### 5. 在中断服务函数里调用 FreeRTOS API 必须注意哪两点？

1. 该中断的优先级必须在 **FreeRTOS 管理范围内**（优先级 5~15，即数值 ≥ `configMAX_SYSCALL_INTERRUPT_PRIORITY` 对应值）；
2. 必须使用**带 `FromISR` 后缀**的函数（`xSemaphoreGiveFromISR` 等），并在需要时根据返回值调用 `portYIELD_FROM_ISR()` 请求切换。普通版 API 在中断里调用可能阻塞或破坏内核状态。

---

## 八、队列

### 1. 队列是什么？相比全局变量做任务间通信有什么优点？

队列是**任务到任务、任务到中断、中断到任务**的"消息传递"机制。

全局变量的弊端：数据无保护，多任务同时操作易被破坏。队列的优点：

- 读写操作内部有**临界区保护**，不会被打断 → **线程安全**；
- 只需调用 API 即可，简单易用；
- 队列是"线程安全的 memcpy 中转站"：按值把完整数据拷贝进出，中间无论发生什么中断/切换数据都纹丝不动。

### 2. 队列有哪些关键特性？

1. **FIFO**：先进先出（默认），也支持 LIFO（`xQueueSendToFront` 插队头）与覆写（`xQueueOverwrite`，仅队列长度为 1）；
2. **固定大小**：创建时定队列长度 `uxQueueLength` 和项目大小 `uxItemSize`；
3. **值拷贝**：数据按值传递（拷贝进队列），大块数据传指针；
4. **阻塞机制**：队空（读）/队满（写）时可阻塞等待，支持超时；
5. 任何任务和中断都可读写队列（中断用 `FromISR` 版）。

### 3. 队列的阻塞时间 `xTicksToWait` 有哪三种语义？

- `0`：不等待，立刻返回（失败就失败）；
- `0 ~ portMAX_DELAY`：最多等待设定时间，超时后直接返回；
- `portMAX_DELAY`：**死等**，一直阻塞到能入队/出队为止（等待永久的阻塞相当于挂起）。

**多个任务同时写一个满队列时的唤醒规则**：队列空出位置后——① 给**优先级最高**的任务；② 若优先级相同，给**等待时间最长**的任务。

### 4. 写出队列常用 API 及其作用、返回值。

| 函数 | 作用 |
| --- | --- |
| `xQueueCreate(length, itemSize)` | 动态创建队列，返回句柄（NULL=失败） |
| `xQueueSend()` / `xQueueSendToBack()` | 写队列**尾部**（两者等价，FIFO） |
| `xQueueSendToFront()` | 写队列**头部**（LIFO 插队） |
| `xQueueOverwrite()` | **覆写**队列（只用于长度 1 的队列） |
| `xQueueReceive(buf, timeout)` | 从队头**读并删除**消息；`pdTRUE` 成功/`pdFALSE` 失败 |
| `xQueuePeek(buf, timeout)` | 从队头**读但不删除** |
| `xQueueSendFromISR()` 等 | 中断版写入（带 `FromISR` 后缀） |

前四个写函数最终都调用同一个 `xQueueGenericSend()`，只是 `xCopyPosition` 参数不同（BACK / FRONT / OVERWRITE）。

### 5. 队列与信号量在底层有什么关系？

**信号量本质就是一个队列**，只是不存数据：

- 二值信号量 = **队列长度为 1、项目大小为 0** 的队列（只有空/满）；
- 计数型信号量 = 队列长度大于 1、项目大小 0 的队列（计数值即"空位/资源"数）；
- 互斥信号量 = 带优先级继承的二值信号量；
- 队列结构体里 `uxMessagesWaiting` 就是信号量的"计数值"。

这也解释了为什么创建队列需两块内存（结构体+存储区），而信号量只需一块（无存储区）。

---

## 九、信号量

### 1. 二值信号量、计数型信号量、互斥信号量各自的用途与创建 API？

| 类型 | 本质/最大计数值 | 典型用途 | 创建函数 |
| --- | --- | --- | --- |
| 二值信号量 | 队列长度 1，只有 0/1 | **任务同步**（中断里 `Give` 通知任务 `Take`） | `xSemaphoreCreateBinary()` |
| 计数型信号量 | 计数上限 `uxMaxCount`（≥1） | **事件计数**（初始 0，每事件 +1）/ **资源管理**（初始 = 资源数，每占用 -1） | `xSemaphoreCreateCounting(max, init)` |
| 互斥信号量 | 带**优先级继承**的二值信号量 | **互斥访问共享资源** | `xSemaphoreCreateMutex()` |

### 2. 释放 `xSemaphoreGive` 与获取 `xSemaphoreTake` 的阻塞行为有何不同？

- **Give（释放）**：**不可阻塞**。计数值 +1；当计数值**已达到最大值**时释放失败（返回 `errQUEUE_FULL`）。
- **Take（获取）**：计数值 -1；**没有资源时可阻塞**等待（`xBlockTime` 指定超时，成功返回 `pdTRUE`，超时 `pdFALSE`）。

二值信号量 Give 底层就是 `xQueueGenericSend(...queueSEND_TO_BACK)`，所以它继承了"满则失败"的语义。

### 3. 什么是优先级翻转？互斥信号量的"优先级继承"如何缓解它？

**优先级翻转**：高优先级任务反而被低优先级任务"拖慢"。

```
任务L(低)先拿到信号量 → 任务H(高)抢占L，但信号量在L手上，H被阻塞
→ 任务M(中)来了：M优先级比L高又不需要信号量，于是 M 插队执行
→ M 跑完，L 才能继续；L 跑完释放信号量，H 才拿到执行
```

**优先级继承**：当高优先级任务 H 被持有互斥信号量的低优先级任务 L 阻塞时，内核**临时把 L 的优先级提升到与 H 相同**，让 L 尽快执行完并释放信号量，从而**把 H 的阻塞时间压缩到只剩 L 的执行时间**。

注意：优先级继承**不能完全消除**优先级翻转，只能尽量降低影响。因为二值信号量没有继承机制、容易翻转，所以二值信号量更适合**同步**，互斥才用互斥信号量。

### 4. 为什么互斥信号量不能用在中断服务函数里？

1. 互斥信号量依赖**优先级继承**机制，而中断不是任务、**没有任务优先级**，无法继承；
2. 中断服务函数**不能阻塞**（不能等待获取），互斥信号量的 Take 可能进入阻塞。

### 5. 为什么"创建互斥信号量后是先获取（Take），而二值信号量是先释放（Give）"？

`xSemaphoreCreateMutex()` 内部**主动释放（Give）了一次**，所以创建出来的互斥信号量初始"有信号量"，要先 `Take` 获得使用权；而二值信号量创建后初始是**空的（0）**，要等中断/任务先 `Give` 才能 `Take`。

---

## 十、事件标志组

### 1. 事件标志组是什么？最多能用多少个事件位？为什么？

事件标志组 = **一组事件标志位的集合，用一个整数存储**（`EventBits_t`，32 位时是 `uint32_t`）。每一位表示一个事件：置 1 表示发生、清 0 表示未发生；事件含义由用户自己定义；任何任务/中断都可读写这些位。

32 位整数中**高 8 位用作控制信息**，所以**低 24 位存储事件标志** → 最多 24 个事件（若 `configUSE_16_BIT_TICKS=1` 则用 16 位 `TickType_t`，相应变少）。

### 2. 事件标志组与队列/信号量相比有什么不同？

| 功能 | 队列、信号量 | 事件标志组 |
| --- | --- | --- |
| 唤醒对象 | 只唤醒**一个**任务 | 唤醒**所有**符合条件的任务（广播） |
| 事件清除 | **消耗型**：数据读走就没了、信号量获取后计数减少 | **非消耗型**：被唤醒的任务可选择**保留**事件，也可用 ClearBits **主动清除** |

### 3. `xEventGroupWaitBits()` 的各参数含义？如何实现"等待多位"？

```c
xEventGroupWaitBits(xEventGroup, uxBitsToWaitFor, xClearOnExit, xWaitForAllBits, xTicksToWait);
```

| 参数 | 含义 |
| --- | --- |
| `uxBitsToWaitFor` | 等待的事件位（多个位用 `|` 逻辑或组合） |
| `xClearOnExit` | 成功等到后是否清除指定位：`pdTRUE` 清 / `pdFALSE` 不清 |
| `xWaitForAllBits` | `pdTRUE`：等待的位**全部为 1**（逻辑与）才返回；`pdFALSE`：只要**某一个为 1**（逻辑或）就返回 |
| `xTicksToWait` | 阻塞超时时间 |

**两种等待方式**：等某一位（或某几位中的任一位）成立、等所有指定位置位（任务同步"会合"常用）。

### 4. 事件标志组的主要 API 有哪些？

- `xEventGroupCreate()` 动态创建；`...Static()` 静态创建；
- `xEventGroupSetBits(组, 位)` 置 1（有 `FromISR` 版）；
- `xEventGroupClearBits(组, 位)` 清 0（有 `FromISR` 版）；
- `xEventGroupWaitBits()` 等待事件；
- `xEventGroupSync()`：**置位并等待**——任务到达同步点先把自己的位 SET 上去，同时等待别人把其它位也 SET（多任务会合/同步屏障用）。

---

## 十一、任务通知

### 1. 任务通知的机制与优势是什么？

任务通知直接使用**任务控制块 TCB 内自带的成员** `ulNotifiedValue`（通知值）+ `ucNotifyState`（通知状态）来通信，**无需额外创建队列/信号量/事件标志组结构体**。

优势：

- **效率高**：比队列/信号量/事件组快得多（约快 45%）；
- **省内存**：不额外创建内核对象，占用最小；
- 可**替代队列、信号量、事件标志组**使用。

### 2. 任务通知的劣势（限制）有哪些？

1. **无法发送数据给 ISR**：ISR 没有任务控制块，不能接收通知；但 **ISR 可以用 `FromISR` 版发送通知给任务**；
2. **无法广播**：通知只能被**指定的一个任务**接收处理（一对一）；
3. **无法缓存多个数据**：TCB 只有一个通知值，只能保存一个数据（新通知覆盖/累加，没有队列的缓存能力）；
4. **发送方不支持阻塞**：发送通知不会让发送任务进入阻塞等待。

### 3. 任务通知值的更新方式有哪几种（`eNotifyAction`）？各模拟什么机制？

| `eNotifyAction` | 动作 | 类似机制 |
| --- | --- | --- |
| `eNoAction` | 只发通知不改值 | 纯信号/唤醒 |
| `eSetBits` | 更新指定 bit（置位） | **事件标志组** |
| `eIncrement` | 通知值 +1 | **信号量**（`xTaskNotifyGive` 即封装此动作） |
| `eSetValueWithOverwrite` | 直接覆写通知值 | 队列/邮箱（覆盖写） |
| `eSetValueWithoutOverwrite` | 有值则不覆盖 | 队列（不覆盖时返回失败） |

### 4. 任务通知的接收函数有哪些？分别在什么场景用？

- `ulTaskNotifyTake(xClearCountOnExit, xTicksToWait)`：**信号量式**接收——把通知值当计数器，退出时可选择清零或减一，返回值是清零/递减**之前**的通知计数值；
- `xTaskNotifyWait(ulBitsToClearOnEntry, ulBitsToClearOnExit, &pulNotificationValue, xTicksToWait)`：**事件标志组/队列式**接收——可指定进入/退出时清除哪些位，并取回通知值。

**注意**：发送通知 API 可用于任务和中断（FromISR 版）；**接收通知 API 只能用于任务**，因为中断没有 TCB。

### 5. 任务通知的状态机有哪三种？

| 状态 | 含义 |
| --- | --- |
| `taskNOT_WAITING_NOTIFICATION`（0） | 任务**未等待通知**（默认初始态） |
| `taskWAITING_NOTIFICATION`（1） | 任务已调用接收函数，**正在等待**通知 |
| `taskNOTIFICATION_RECEIVED`（2） | 发送方已发通知，**等待接收方取走** |

---

## 十二、队列集

### 1. 队列集解决了什么问题？

单个队列只能传**同一种类型**的消息。队列集 = 把**多个队列/信号量"监听"起来**：无论哪一个队列/信号量有消息，等待队列集的任务都会**退出阻塞**，再通过返回值判断是哪个队列来的消息——相当于"多路复用监听"（类似 select/epoll 的思路），一个任务就能统一处理多路消息。

### 2. 队列集有哪些 API？使用流程是什么？

| API | 作用 |
| --- | --- |
| `xQueueCreateSet(uxEventQueueLength)` | 创建队列集（参数 = 可容纳的队列/信号量个数） |
| `xQueueAddToSet(队列, 队列集)` | 把队列/信号量加入队列集（`pdPASS`/`pdFAIL`） |
| `xQueueRemoveFromSet(队列, 队列集)` | 从队列集移除 |
| `xQueueSelectFromSet(队列集, 超时)` | 阻塞等待，返回**有有效消息的队列句柄**（NULL=失败） |
| `xQueueSelectFromSetFromISR()` | 中断版本 |

使用流程：

1. `configUSE_QUEUE_SETS` 置 1；
2. 创建队列集；
3. 创建队列/信号量；
4. 把队列/信号量加入队列集；
5. 往队列发消息或释放信号量；
6. 用 `xQueueSelectFromSet` 获取"有消息的那个队列"再读取。

> ⚠️ 重要前提：队列**在加入队列集之前必须为空**（不能有有效消息），否则添加会失败。

---

## 十三、软件定时器

### 1. 软件定时器与硬件定时器有什么区别？各自的优缺点？

| | 硬件定时器 | 软件定时器 |
| --- | --- | --- |
| 本质 | 芯片自带定时器模块，到时**触发中断**，用户在 ISR 处理 | 具有定时功能的软件，到时**调用回调函数（超时函数）** |
| 数量 | 有限（芯片定时器外设就那么几个） | 理论上只要内存够可建多个 |
| 精度 | 高 | 相对低（以系统 tick 为基准，tick 中断优先级最低易被打断） |
| 成本 | 占用硬件资源 | 简单、成本低 |

对精度要求高的场合不建议用软件定时器。

### 2. 软件定时器的"服务任务"和"命令队列"是什么？

- **软件定时器服务任务（守护任务）**：在 `vTaskStartScheduler()` 启动调度器时自动创建（前提 `configUSE_TIMERS=1`），优先级 `configTIMER_TASK_PRIORITY`（例程 = 31，最高）。职责：① 软件定时器超时逻辑判断；② 调用超时回调函数；③ 处理软件定时器命令队列。
- **命令队列**：所有软件定时器 API（Start/Stop/Reset/ChangePeriod 等）本质都是**往命令队列发消息**，由服务任务取出执行；命令队列长度为 `configTIMER_QUEUE_LENGTH`（例程 5），**用户不能直接访问**。

### 3. 软件定时器的回调函数里为什么不能调用阻塞 API？

回调函数**由软件定时器服务任务调用**，而回调本身不是任务。服务任务是**所有软件定时器共享**的，若某个回调里调用 `vTaskDelay()` 或带非零阻塞时间的队列/信号量函数导致阻塞，会**卡住整个服务任务，影响其它所有软件定时器**。所以回调要求：尽快执行、不阻塞。

### 4. 单次定时器与周期定时器有什么区别？新创建的定时器处于什么状态？

- **单次定时器**：到时只执行**一次**回调，不自动重启（可手动 `xTimerStart` 再次开启）；
- **周期定时器**：每次回调执行完**自动重新开始**，周期性触发（`uxAutoReload` 决定）。

软件定时器有**休眠态/运行态**两态：**新创建的定时器处于休眠态（未运行）**，需要调 `xTimerStart()`（本质是发命令）才转入运行态开始计时。

### 5. 软件定时器主要 API 有哪些？

- `xTimerCreate(name, periodTicks, uxAutoReload, pvTimerID, callback)` 创建；
- `xTimerStart(timer, xTicksToWait)` 开启（休眠→运行）；
- `xTimerStop(timer, xTicksToWait)` 停止；
- `xTimerReset(timer, xTicksToWait)` 复位（以复位时刻为起点重新定时）；
- `xTimerChangePeriod(timer, newPeriod, xTicksToWait)` 改周期。

`xTicksToWait` 是"发送命令到命令队列时的阻塞等待时间"（命令队列满时可等待）。

---

## 十四、内存管理

### 1. FreeRTOS 为什么不用标准 C 库的 `malloc/free`？提供了哪 5 种内存管理算法？

C 库动态内存的缺点：① 占用大量代码空间（不适合资源紧缺的嵌入式系统）；② **没有线程安全机制**；③ 运行时间不确定；④ 会产生内存碎片。

FreeRTOS 提供 **heap_1 ~ heap_5** 五种算法，按工程需求选用（例程用 **heap_4**）。相关 API：`pvPortMalloc()` / `vPortFree()` / `xPortGetFreeHeapSize()`；堆大小由 `configTOTAL_HEAP_SIZE` 决定（例程 10KB）。

### 2. heap_1 ~ heap_5 各有什么特点？怎么选？

| 算法 | 特点 | 适用 |
| --- | --- | --- |
| heap_1 | 最简单：一个大数组 `ucHeap[]` 顺序切割，**只能申请不能释放**（没有 `vPortFree`） | 任务/对象创建后从不删除的工程 |
| heap_2 | 最适应算法 + 支持释放；但**不合并相邻空闲块** → 会产生碎片 | 频繁创建删除且栈大小相同的场景（无碎片问题） |
| heap_3 | 包装标准 C 库 malloc/free，只是加了调度器挂起保护 | 一般不用 |
| heap_4 | **首次适应算法** + 支持释放 + **把相邻空闲块合并成大块** → 减少碎片（例程用它） | 频繁分配/释放不同大小内存 |
| heap_5 | 在 heap_4 基础上支持**管理多个不连续内存区域**（用 `HeapRegion_t` 数组 + `vPortDefineHeapRegions()` 指定） | 内存地址不连续的芯片 |

**最适应 vs 首次适应**：heap_2 找"能装下且最小的空闲块"（从大到小排好找最小可用）；heap_4 按地址顺序找"第一个能装下的块"。

### 3. 动态创建与静态创建的内存管理方式有何不同？

- 动态：TCB/栈从 FreeRTOS 堆自动申请，删除对象后内存**自动释放回堆**（灵活）；
- 静态：内存由用户提供（全局数组/变量），**占用固定**；对象删除后这些内存一般不做他用，需要用户自己管理。

---

## 十五、低功耗 Tickless 模式

### 1. Tickless 模式的设计思想是什么？为什么要它？

系统运行中大部分时间在执行**空闲任务**（所有其它任务都阻塞/挂起时才运行）。Tickless 的思路：**在"本该空闲任务运行"的时间段让 MCU 进入低功耗模式（睡眠）**，等有任务要运行时再唤醒。本质是调用 **WFI 指令进入睡眠模式**，任何中断/事件可唤醒。

难点有二：① 进入低功耗后**多久唤醒**（下一个要运行的任务如何被准确唤醒）；② SysTick 若频繁中断会破坏低功耗效果。**解决**：空闲期间把 SysTick 中断周期改为"预计睡眠时长"，退出低功耗后**补上错过的系统节拍数**——这些机制 FreeRTOS 已封装好。

### 2. 进入 Tickless 需要满足哪些条件？相关配置宏有哪些？

进入条件：

1. `FreeRTOSConfig.h` 中 `configUSE_TICKLESS_IDLE` 置 1；
2. **空闲任务正在运行**，所有其它任务处于挂起或阻塞态；
3. 可睡眠的节拍数 **≥ `configEXPECTED_IDLE_TIME_BEFORE_SLEEP`**（默认 2 个 tick）。

| 宏 | 作用 |
| --- | --- |
| `configUSE_TICKLESS_IDLE` | 使能 Tickless 低功耗 |
| `configEXPECTED_IDLE_TIME_BEFORE_SLEEP` | 进入低功耗所需的最短空闲节拍数 |
| `configPRE_SLEEP_PROCESSING(x)` | 进低功耗**前**执行：关闭外设时钟、降低主频等（进一步省电） |
| `configPOST_SLEEP_PROCESSING(x)` | 退出低功耗**后**执行：重新开启外设时钟、恢复主频 |

STM32 三种低功耗模式（睡眠/停止/待机）中 FreeRTOS 主要用**睡眠模式**（WFI/WFE 进入，任何中断唤醒）。

---

## 十六、FreeRTOSConfig.h 系统配置

### 1. `FreeRTOSConfig.h` 里的宏大致分哪三类？各举一例。

1. **INCLUDE_ 类**：配置 FreeRTOS 中可选的 API 函数是否编译（省 Flash）。如 `INCLUDE_vTaskDelete`、`INCLUDE_vTaskSuspend`、`INCLUDE_uxTaskGetStackHighWaterMark`；
2. **config_ 类**：完成 FreeRTOS 的功能配置和裁剪。如 `configUSE_PREEMPTION`、`configTICK_RATE_HZ`、`configTOTAL_HEAP_SIZE`、`configUSE_TIMERS`；
3. **其它**：PendSV/SVC 宏定义（`xPortPendSVHandler → PendSV_Handler`）、断言 `configASSERT` 等。

### 2. 说明下列核心宏的作用：

| 宏（例程值） | 作用 |
| --- | --- |
| `configUSE_PREEMPTION = 1` | 抢占式调度（1）还是协程式（0） |
| `configTICK_RATE_HZ = 1000` | SysTick 节拍频率，1000Hz = 每 1ms 一个 tick |
| `configMAX_PRIORITIES = 32` | 最大优先级数量（0~31） |
| `configMINIMAL_STACK_SIZE = 128` | 空闲任务栈大小（单位 word，128×4=512B） |
| `configTOTAL_HEAP_SIZE = 10KB` | FreeRTOS 堆总大小（任务栈/TCB/队列/信号量都从这分配） |
| `configSUPPORT_STATIC_ALLOCATION = 0` | 是否支持静态创建（1 时需要用户提供 TCB/栈 + 内核内存回调） |
| `configSUPPORT_DYNAMIC_ALLOCATION = 1` | 是否支持动态创建（xTaskCreate 从堆分配） |
| `configUSE_TIMERS = 1` | 使能软件定时器（启动调度器时自动创建定时器服务任务） |
| `configUSE_MUTEXES / _COUNTING_SEMAPHORES / _QUEUE_SETS` | 分别使能互斥信号量、计数信号量、队列集 |
| `configCHECK_FOR_STACK_OVERFLOW` | 栈溢出检测（0 关；2 = 水印法，开发期推荐） |

### 3. 中断相关宏 `configPRIO_BITS`、`configKERNEL_INTERRUPT_PRIORITY`、`configMAX_SYSCALL_INTERRUPT_PRIORITY` 的含义与换算？

- `configPRIO_BITS = 4`：STM32 中断优先级只用寄存器高 4 位 → 0~15 共 16 级（CMSIS 提供时直接取 `__NVIC_PRIO_BITS`）；
- `configLIBRARY_LOWEST_INTERRUPT_PRIORITY = 15`：最低中断优先级；
- `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`：FreeRTOS 系统调用能安全使用的最高优先级（5~15 可调 API，0~4 不行）；
- 左移 4 位得寄存器原始值：`configKERNEL_INTERRUPT_PRIORITY = 15<<4 = 0xF0`（SysTick/PendSV 用，最低）；`configMAX_SYSCALL_INTERRUPT_PRIORITY = 5<<4 = 0x50`（BASEPRI 屏蔽阈值）。

**记忆**：FreeRTOS 内部把优先级放高 4 位是因为 STM32 低 4 位不用；`BASEPRI=0x50` 即屏蔽 5~15，放行 0~4。

### 4. 栈溢出检测 `configCHECK_FOR_STACK_OVERFLOW` 的两种方法区别？调试栈大小用哪个 API？

- 方法 1：检测**栈顶**是否被破坏（任务切换时检查）；
- 方法 2：**水印法**——记录任务栈历史最小剩余值（开发时推荐），配合 `uxTaskGetStackHighWaterMark(xTask)` 读取"高水位线"，返回值**越小越接近栈溢出**，是给任务定栈大小的利器。

---

## 十七、任务相关 API（查询/统计）

### 1. 列举常用的任务查询类 API 及其作用。

| API | 作用 | 前提宏 |
| --- | --- | --- |
| `uxTaskPriorityGet(xTask)` | 获取任务优先级（传 NULL = 当前任务） | `INCLUDE_uxTaskPriorityGet` |
| `vTaskPrioritySet(xTask, prio)` | 设置任务优先级 | `INCLUDE_vTaskPrioritySet` |
| `uxTaskGetNumberOfTasks()` | 获取系统中任务总数 | — |
| `xTaskGetCurrentTaskHandle()` | 获取当前运行任务的句柄 | `INCLUDE_xTaskGetCurrentTaskHandle` |
| `xTaskGetHandle(name)` | 通过任务名获取句柄 | `INCLUDE_xTaskGetHandle` |
| `eTaskGetState(xTask)` | 查询任务状态（`eRunning/eReady/eBlocked/eSuspended/eDeleted/eInvalid`） | `INCLUDE_eTaskGetState` |
| `uxTaskGetStackHighWaterMark(xTask)` | 获取任务栈历史最小剩余（word 数，越小越危险） | `INCLUDE_uxTaskGetStackHighWaterMark` |
| `vTaskList(buf)` | 表格形式列出所有任务（Name/State/优先级/栈水位/Num） | `configUSE_TRACE_FACILITY` + `configUSE_STATS_FORMATTING_FUNCTIONS` |

`vTaskList` 的状态字段：`B`=阻塞、`R`=就绪、`S`=挂起、`D`=删除。

### 2. 任务运行时间统计如何开启？需要额外提供什么？

步骤：

1. `configGENERATE_RUN_TIME_STATS = 1`；
2. `configUSE_STATS_FORMATTING_FUNCTIONS = 1`；
3. 实现两个宏/函数：
   - `portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()`：初始化一个**比系统 tick 精度高 10~100 倍**的时基定时器（例程用 TIM6 配 10kHz）；
   - `portGET_RUN_TIME_COUNTER_VALUE()`：返回该定时器的计数值（如 `FreeRTOSRunTimeTicks`）。

然后 `vTaskGetRunTimeStats(buf)` 即可打印各任务的 **Abs Time（绝对运行时间）与 % Time（CPU 占用百分比）**，用于分析哪个任务吃 CPU 多、验证延时/阻塞设计是否合理。

### 3. 为什么要检查任务"运行时间统计"和"栈高水位"这两类数据？各解决什么问题？

- 运行时间统计：发现**某任务占 CPU 过多**或空闲率过低——用于调整优先级、任务拆分、减少空转（空闲任务占比高 = CPU 很闲，是 Tickless 低功耗的前提依据）；
- 栈高水位：发现**某任务栈快不够用**（返回值趋近 0）→ 调大该任务栈大小，防止栈溢出导致的随机崩溃。两者都是 FreeRTOS 开发期最重要的调优/排障手段。

---

## 附：核心易混点速记

| 易混点 | 记忆 |
| --- | --- |
| 任务优先级 vs 中断优先级 | 任务：**数大优先高**；NVIC 中断：**数小优先高** |
| 挂起 vs 删除 | 挂起可恢复、保留上下文；删除清栈、不可恢复 |
| 删除他人 vs 删自己 | 删他人：调用处释放；删自己(NULL)：**空闲任务**里释放 |
| 临界区 vs 挂起调度器 | 临界区=**关中断**；`vTaskSuspendAll`=**不关中断**只禁调度 |
| 二值 vs 互斥信号量 | 二值无继承 → 适合**同步**；互斥有**优先级继承** → 适合**互斥**且**不能在中断用** |
| 队列/信号量 vs 事件标志组 | 前者**消耗型、唤醒一个**；事件组**广播、可保留**事件 |
| 动态 vs 静态创建 | 动态自动从堆分配；静态用户给 TCB+栈，且要管空闲/定时器任务内存回调 |
| `vTaskDelay` vs `vTaskDelayUntil` | 相对（从调用起算，周期不固定）vs 绝对（固定周期，主体耗时须小于周期） |
| `vListInsert` vs `vListInsertEnd` | 按 `xItemValue` 升序插 vs 插在 `pxIndex` 前（时间片轮转用） |
| 定时器/队列/信号量 vs 任务通知 | 通知=TCB 自带、更快更省，但**不能发给 ISR、不能广播、不能缓存多个** |
| 软件定时器回调 | 在**服务任务**中执行、不是任务 → **禁止阻塞** API |

---

*答案整理自你的 FreeRTOS 笔记（20 篇），与正点原子 F103 例程一致。用于查缺补漏，发现与手册/实验现象冲突时以实测为准。*
