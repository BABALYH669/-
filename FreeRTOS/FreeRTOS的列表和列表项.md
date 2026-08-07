---
notion-id: 37c5d378-8ef5-80a7-85af-f6b9be88daa7
---
# 列表和列表项简介

列表是 FreeRTOS 中的一个数据结构，概念上和链表有点类似，列表被用来跟踪 FreeRTOS中的任务。

列表项就是存放在列表中的项目


![[wps_WwVc5P1X1o.png]]

列表相当于链表，列表项相当于节点，FreeRTOS 中的列表是一个双向环形链表

列表的特点：列表项间的地址非连续的，是人为的连接到一起的。列表项的数目是由后期添加的个数决定的，随时可以改变

数组的特点：数组成员地址是连续的，数组在最初确定了成员数量后期无法改变

在OS中任务的数量是不确定的，并且任务状态是会发生改变的，所以非常适用列表(链表)这种数据结构

![[wps_90HIfnu0x9.png]]

1. 在该结构体中， 包含了两个宏，这两个宏是确定的已知常量， FreeRTOS通过检查这两个常量的值，来判断列表的数据在程序运行过程中，是否遭到破坏 ，该功能一般用于调试， 默认是不开启的
2. 成员uxNumberOfItems，用于记录列表中列表项的个数（不包含 xListEnd）
3. 成员 pxIndex 用于指向列表中的某个列表项，一般用于遍历列表中的所有列表项 
4. 成员变量 xListEnd 是一个迷你列表项，排在最末尾

![[wps_g1itd3IuMa.png]]

5.  成员变量 xItemValue 为列表项的值，这个值多用于按升序对列表中的列表项进行排序
6. 成员变量 pxNext 和 pxPrevious 分别用于指向列表中列表项的下一个列表项和上一个列表项 
7. 成员变量 pxOwner 用于指向包含列表项的对象（通常是任务控制块）
8. 成员变量 pxContainer 用于指向列表项所在列表。

迷你列表项也是列表项，但迷你列表项仅用于标记列表的末尾和挂载其他插入列表中的列表项 

![[wps_OEHiAV7pyI.png]]

9. 成员变量 xItemValue 为列表项的值，这个值多用于按升序对列表中的列表项进行排序
10. 成员变量 pxNext 和 pxPrevious 分别用于指向列表中列表项的下一个列表项和上一个列表项 
11. 迷你列表项只用于标记列表的末尾和挂载其他插入列表中的列表项，因此不需要成员变量 pxOwner 和 pxContainer，以节省内存开销


![[wps_XwzGPUIDya.png]]

# 列表相关API函数介绍

| **函数** | **描述** |
| --- | --- |
| vListInitialise() | 初始化列表 |
| vListInitialiseItem() | 初始化列表项 |
| vListInsertEnd() | 列表末尾插入列表项 |
| vListInsert() | 列表插入列表项 |
| uxListRemove() | 列表移除列表项 |

![[wps_x38PxUoEzh.png]]

```c
/*函数原型*/
void vListInitialise(List_t * const pxList);

void vListInitialise(
			List_t * const pxList)
{
		/* 初始化时，列表中只有 xListEnd，因此 pxIndex 指向 xListEnd */
		pxList->pxIndex = ( ListItem_t * ) &( pxList->xListEnd );
		
		/* xListEnd 的值初始化为最大值，用于列表项升序排序时，排在最后 */
		/*portMAX_DELAY = 0xFFFFFFFF*/
		pxList->xListEnd.xItemValue = portMAX_DELAY;
		
		/* 初始化时，列表中只有 xListEnd，因此上一个和下一个列表项都为 xListEnd 本身 */
		pxList->xListEnd.pxNext = ( ListItem_t * ) &( pxList->xListEnd );
		pxList->xListEnd.pxPrevious = ( ListItem_t * ) &( pxList->xListEnd );
		
		/*初始化时，列表中的列表项数量为 0（不包含 xListEnd） */
		pxList->uxNumberOfItems = ( UBaseType_t ) 0U;
		
		/* 初始化用于检测列表数据完整性的校验值 */
		listSET_LIST_INTEGRITY_CHECK_1_VALUE( pxList );
		listSET_LIST_INTEGRITY_CHECK_2_VALUE( pxList );
}
```

![[wps_xZpYcrLWM9.png]]

```c
/*函数原型*/
void vListInitialiseItem(ListItem_t * const pxItem);

void vListInitialiseItem(
		ListItem_t * const pxItem)
{
		/* 初始化时，列表项所在列表设为空 */
		pxItem->pxContainer = NULL;
		
		/* 初始化用于检测列表项数据完整性的校验值 */
		listSET_FIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem );
		listSET_SECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem );
}
```

![[wps_5i1Lc4yrBU.png]]

```c
/*函数原型*/
void vListInsert(
		List_t * const pxList,
		ListItem_t * const pxNewListItem);
		
void vListInsert(
		List_t * const pxList,
		ListItem_t * const pxNewListItem)
{
		ListItem_t * pxIterator;
		const TickType_t xValueOfInsertion = pxNewListItem->xItemValue;
		
		/* 检查参数是否正确 */
		listTEST_LIST_INTEGRITY( pxList );
		listTEST_LIST_ITEM_INTEGRITY( pxNewListItem );
		
		/* 如果待插入列表项的值为最大值 */
		if( xValueOfInsertion == portMAX_DELAY )
		{
				/* 插入的位置为列表 xListEnd 前面 */
				pxIterator = pxList->xListEnd.pxPrevious;
		}
		else
		{
				/* 遍历列表中的列表项，找到插入的位置 */
				for( pxIterator = ( ListItem_t * ) &( pxList->xListEnd );
				pxIterator->pxNext->xItemValue <= xValueOfInsertion;
				pxIterator = pxIterator->pxNext )
				{
				
				}
		}
		
		/* 将待插入的列表项插入指定位置 */
		pxNewListItem->pxNext = pxIterator->pxNext;
		pxNewListItem->pxNext->pxPrevious = pxNewListItem;
		pxNewListItem->pxPrevious = pxIterator;
		pxIterator->pxNext = pxNewListItem;
		
		/* 更新待插入列表项所在列表 */
		pxNewListItem->pxContainer = pxList;
		
		/* 更新列表中列表项的数量 */
		( pxList->uxNumberOfItems )++;
}
```

![[wps_87evh1s8fX.png]]

![[wps_6VNJGDbROe.png]]

![[wps_ngkaTVN4BC.png]]

![[wps_SHd1omN9Jj.png]]

![[wps_OlWBUvArWJ.png]]

```c
/*函数原型*/
void vListInsertEnd(
		List_t * const pxList,
		ListItem_t * const pxNewListItem);
		

void vListInsertEnd(
		List_t * const pxList,
		ListItem_t * const pxNewListItem)
{
		/* 获取列表 pxIndex 指向的列表项 */
		ListItem_t * const pxIndex = pxList->pxIndex;
		
		/* 检查参数是否正确 */
		listTEST_LIST_INTEGRITY( pxList );
		listTEST_LIST_ITEM_INTEGRITY( pxNewListItem );
		
		/* 更新待插入列表项的指针成员变量 */
		pxNewListItem->pxNext = pxIndex;
		pxNewListItem->pxPrevious = pxIndex->pxPrevious;
		
		/* 测试使用，不用理会 */
		mtCOVERAGE_TEST_DELAY();
		
		/* 更新列表中原本列表项的指针成员变量 */
		pxIndex->pxPrevious->pxNext = pxNewListItem;
		pxIndex->pxPrevious = pxNewListItem;
		
		/* 更新待插入列表项的所在列表成员变量 */
		pxNewListItem->pxContainer = pxList;
		
		/* 更新列表中列表项的数量 */
		( pxList->uxNumberOfItems )++;
}
```

![[wps_7CZeeTKVO2.png]]

![[wps_mMnOzB0i4F.png]]

![[wps_BYa5dOQCNt.png]]

![[wps_BTkNVqe9vR.png]]

![[wps_zuHn77ONuR.png]]

```c
/*函数原型*/
UBaseType_t uxListRemove(
		ListItem_t * const pxItemToRemove);
		
UBaseType_t uxListRemove(
		ListItem_t * const pxItemToRemove)
{
		List_t * const pxList = pxItemToRemove->pxContainer;
		
		/* 从列表中移除列表项 */
		pxItemToRemove->pxNext->pxPrevious = pxItemToRemove->pxPrevious;
		pxItemToRemove->pxPrevious->pxNext = pxItemToRemove->pxNext;
		
		/* 测试使用，不用理会 */
		mtCOVERAGE_TEST_DELAY();
		
		/* 如果 pxIndex 正指向待移除的列表项 */
		if( pxList->pxIndex == pxItemToRemove )
		{
				/* pxIndex 指向上一个列表项 */
				pxList->pxIndex = pxItemToRemove->pxPrevious;
		}
		else
		{
				mtCOVERAGE_TEST_MARKER();
		}
		
		/* 将待移除列表项的所在列表指针清空 */
		pxItemToRemove->pxContainer = NULL;
		
		/* 更新列表中列表项的数量 */
		( pxList->uxNumberOfItems )--;
		
		/* 返回列表项移除后列表中列表项的数量 */
		return pxList->uxNumberOfItems;
}
```

![[wps_BwujU1188w.png]]

![[wps_gxjuXqgZcL.png]]

| 宏定义 | 描述 |
| --- | --- |
| listSET_LIST_ITEM_OWNER( pxListItem, pxOwner ) | 设置列表项的拥有者 |
| listGET_LIST_ITEM_OWNER( pxListItem ) | 获取列表项的拥有者 |
| listSET_LIST_ITEM_VALUE( pxListItem, xValue ) | 设置列表项的值 |
| listGET_LIST_ITEM_VALUE( pxListItem ) | 获取列表项的值 |
| listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxList ) | 获取列表头部列表项的值 |
| listGET_HEAD_ENTRY( pxList ) | 获取列表的头部列表项 |
| listGET_NEXT( pxListItem ) | 获取列表项的下一个列表项 |
| listGET_END_MARKER( pxList ) | 获取列表的尾部列表项 |
| listLIST_IS_EMPTY( pxList ) | 判断列表是否为空 |
| listCURRENT_LIST_LENGTH( pxList ) | 获取列表包含的列表项数量 |
| listGET_OWNER_OF_NEXT_ENTRY( pxTCB, pxList ) | 获取下一个列表项的拥有者 |
| listREMOVE_ITEM( pxItemToRemove ) | 将列表项从列表中移除 |
| listINSERT_END( pxList, pxNewListItem ) | 列表末尾插入列表项 |
| listGET_OWNER_OF_HEAD_ENTRY( pxList ) | 获取列表头部列表项的拥有者 |
| listIS_CONTAINED_WITHIN( pxList, pxListItem ) | 判断列表项是否在列表中 |
| listLIST_ITEM_CONTAINER( pxListItem ) | 获取列表项所在列表 |
| listLIST_IS_INITIALISED( pxList ) | 判断列表是否完成初始化 |

# 列表项的插入和删除实验

# FreeRTOS 列表和列表项实验 — 完整详解

---

## 第一步：数据结构 — 先看懂"积木"长什么样

这个实验直接操作 FreeRTOS 内核的三个核心数据结构（在 `list.h` 中定义）。

### 1.1 `ListItem_t` — 列表项（链表节点）

```c
struct xLIST_ITEM
{
    TickType_t   xItemValue;     // ★ 排序值（按此值升序插入）
    struct xLIST_ITEM * pxNext;     // 指向前一个列表项
    struct xLIST_ITEM * pxPrevious; // 指向后一个列表项
    void * pvOwner;              // ★ 指向"谁拥有这个列表项"（通常是 TCB）
    struct xLIST * pxContainer;  // ★ 指向"我在哪个列表里"
};
typedef struct xLIST_ITEM ListItem_t;
```

**关键点**：`xItemValue` 决定了插入位置，值越小越靠前。`pvOwner` 是"反向指针"——通过列表项能直接找到拥有它的对象（比如任务控制块）。

### 1.2 `MiniListItem_t` — 迷你列表项（哨兵/终点标记）

```c
struct xMINI_LIST_ITEM
{
    TickType_t   xItemValue;     // 固定为 portMAX_DELAY（最大值 0xFFFFFFFF）
    struct xLIST_ITEM * pxNext;
    struct xLIST_ITEM * pxPrevious;
};
typedef struct xMINI_LIST_ITEM MiniListItem_t;
```

**xItemValue 永远是最大值**，所以不管插入什么，它永远在末尾当"终点站"。

### 1.3 `List_t` — 列表（链表头）

```c
typedef struct xLIST
{
    UBaseType_t    uxNumberOfItems;  // 当前列表里有多少个列表项
    ListItem_t *   pxIndex;          // ★ 游标：遍历列表用
    MiniListItem_t xListEnd;         // ★ 哨兵：列表终点标记
} List_t;
```

`**xListEnd**`** 是列表自带的迷你列表项，值为最大值，永远在最后。它是环形链表的"闭合点"。**

---

## 第二步：画图 — 一个空列表的内存布局

执行 `vListInitialise(&TestList)` 后：

```plain text
     TestList（列表头）
    ┌─────────────────────────────┐
    │  uxNumberOfItems = 0        │
    │  pxIndex ─────────────────────┐
    │  xListEnd:                  │ │
    │    xItemValue = 0xFFFFFFFF  │ │
    │    pxNext ────┐             │ │
    │    pxPrevious ──┐           │ │
    └───────────────┼─┼───────────┘ │
                    │ │             │
                    ▼ ▼             │
     ┌──────────────────────────┐   │
     │     xListEnd 自己指向自己 │◀──┘
     │   pxNext    → 自己       │
     │   pxPrevious → 自己      │
     └──────────────────────────┘
```

**空列表 = 哨兵自己指自己，形成环。**

---

## 第三步：逐步骤讲解代码

### 3.1 变量声明（第 26 ~ 29 行）

```c
List_t      TestList;       // 一个列表
ListItem_t  ListItem1;      // 列表项 1（xItemValue = 40）
ListItem_t  ListItem2;      // 列表项 2（xItemValue = 60）
ListItem_t  ListItem3;      // 列表项 3（xItemValue = 50）
```

### 3.2 task2 — 核心实验代码

task1 只是闪 LED 不相关，所有实验在 task2 里完成。

### 初始化（第 73 ~ 79 行）

```c
vListInitialise(&TestList);       // ① 初始化列表：uxNumberOfItems=0, xListEnd 自环
vListInitialiseItem(&ListItem1);  // ② 初始化列表项1：pxContainer=NULL(表示不在任何链表)
vListInitialiseItem(&ListItem2);  // ③ 列表项2
vListInitialiseItem(&ListItem3);  // ④ 列表项3
ListItem1.xItemValue = 40;        // ⑤ 给三个列表项赋值（决定着插入顺序）
ListItem2.xItemValue = 60;
ListItem3.xItemValue = 50;
```

---

### 第二步：打印地址（第 81 ~ 89 行）

```plain text
TestList            0x20001234
TestList->pxIndex   0x2000123C   ← 指向 xListEnd
TestList->xListEnd  0x2000123C   ← 同一个地址！pxIndex 初始指向哨兵
ListItem1           0x20001250
ListItem2           0x20001270
ListItem3           0x20001290
```

**目的**：让你亲眼看到每个结构在内存里的地址，后面插入时通过地址变化理解链表怎么连起来的。

---

### 第三步：插入列表项 1（第 91 ~ 99 行）

```c
vListInsert(&TestList, &ListItem1);  // 按值 40 插入
```

`**vListInsert**`** 内部逻辑（升序插入）**：

```plain text
从 xListEnd 往后找，找到第一个 xItemValue > 40 的位置，插在它前面。
xListEnd 值是 0xFFFFFFFF > 40 → 插在 xListEnd 前面。
```

插入后的链表：

```plain text
          ┌─── xListEnd ──┐        ┌── ListItem1(val=40) ──┐
          │  pxNext ──────┼────────▶│  pxNext ──────────────┼──┐
          │  pxPrevious ◀─┼────────│  pxPrevious ◀─────────┼──┘
          └────────────────┘        └───────────────────────┘
```

打印出的指针关系：

```plain text
TestList->xListEnd->pxNext     = ListItem1 的地址   ← 哨兵的下一个 = 列表项1
ListItem1->pxNext              = xListEnd 的地址   ← 列表项1的下一个 = 哨兵（环形）
TestList->xListEnd->pxPrevious = ListItem1 的地址   ← 哨兵的前一个 = 列表项1
ListItem1->pxPrevious          = xListEnd 的地址   ← 列表项1的前一个 = 哨兵
```

---

### 第四步：插入列表项 2（第 101 ~ 111 行）

```c
vListInsert(&TestList, &ListItem2);  // 值 = 60
```

**排序过程**：从 xListEnd 往后找 → 第一个遇到 ListItem1(val=40) → 40 < 60，继续 → 遇到 xListEnd(val=MAX) → MAX > 60 → 插在 xListEnd 前面。

```plain text
          ┌─ xListEnd ─┐     ┌─ ListItem1(40) ─┐     ┌─ ListItem2(60) ─┐
          │  pxNext ───┼────▶│  pxNext ────────┼────▶│  pxNext ────────┼──┐
          │  pxPrev ◀──┼────│  pxPrev ◀───────┼────│  pxPrev ◀───────┼──┘
          └────────────┘     └─────────────────┘     └─────────────────┘
```

**升序链**：xListEnd → ListItem1(40) → ListItem2(60) → xListEnd

---

### 第五步：插入列表项 3（第 113 ~ 125 行）

```c
vListInsert(&TestList, &ListItem3);  // 值 = 50
```

**排序过程**：从 xListEnd 往后 → ListItem1(40) → 40 < 50，继续 → ListItem2(60) → 60 > 50 → **插在 ListItem2 前面！**

```plain text
         ┌─ xListEnd ─┐   ┌─ ListItem1(40) ┐   ┌─ ListItem3(50) ┐   ┌─ ListItem2(60) ┐
         │  pxNext ───┼──▶│  pxNext ───────┼──▶│  pxNext ───────┼──▶│  pxNext ───────┼─┐
         │  pxPrev ◀──┼──│  pxPrev ◀──────┼──│  pxPrev ◀──────┼──│  pxPrev ◀──────┼─┘
         └────────────┘   └───────────────┘   └───────────────┘   └───────────────┘
```

**最终升序链**：xListEnd → ListItem1(40) → ListItem3(50) → ListItem2(60) → xListEnd

**核心结论**：`vListInsert` 按 `xItemValue` 升序自动排序，你只管给值，FreeRTOS 帮你找插入位置。

---

### 第六步：移除列表项 2（第 127 ~ 136 行）

```c
uxListRemove(&ListItem2);  // 把自己从链表中摘掉
```

`**uxListRemove**`** 内部逻辑**（从 `list.h` 宏 `listREMOVE_ITEM`）：

```c
// 把 ListItem2 的前后邻居直接连起来，跳过 ListItem2
ListItem2->pxNext->pxPrevious = ListItem2->pxPrevious;  // ListItem3->pxPrev = xListEnd
ListItem2->pxPrevious->pxNext = ListItem2->pxNext;      // xListEnd->pxNext = ListItem3
ListItem2->pxContainer = NULL;  // 标记：我不在任何列表中
```

移除后：

```plain text
         ┌─ xListEnd ─┐   ┌─ ListItem1(40) ┐   ┌─ ListItem3(50) ┐
         │  pxNext ───┼──▶│  pxNext ───────┼──▶│  pxNext ───────┼──┐
         │  pxPrev ◀──┼──│  pxPrev ◀──────┼──│  pxPrev ◀──────┼──┘
         └────────────┘   └───────────────┘   └───────────────┘

         ListItem2 孤立在外，pxContainer = NULL
```

---

### 第七步：末尾插入列表项 2（第 138 ~ 152 行）

```c
TestList.pxIndex = &ListItem1;            // 手动把游标指向 ListItem1
vListInsertEnd(&TestList, &ListItem2);    // 在 pxIndex 指向的位置"后面"插入
```

`**vListInsertEnd**`** 和 **`**vListInsert**`** 的区别**：

|   | `vListInsert` | `vListInsertEnd` |
| --- | --- | --- |
| 插入位置 | 按 `xItemValue` 升序 | 插在 `pxIndex` 之前 |
| 用途 | 按值排序 | 按时间顺序（FIFO 感觉） |
| 典型场景 | 就绪列表（按优先级） | 延时列表（先到先出） |

pxIndex 指向 ListItem1，所以 ListItem2 插在 ListItem1 之前：

```plain text
                                  pxIndex 指向这里
                                       │
         ┌─ xListEnd ─┐   ┌─ ListItem2(60) ┐   ┌─ ListItem1(40) ┐   ┌─ ListItem3(50) ┐
         │  pxNext ───┼──▶│  pxNext ───────┼──▶│  pxNext ───────┼──▶│  pxNext ───────┼─┐
         │  pxPrev ◀──┼──│  pxPrev ◀──────┼──│  pxPrev ◀──────┼──│  pxPrev ◀──────┼─┘
         └────────────┘   └───────────────┘   └───────────────┘   └───────────────┘
```

**顺序不再是升序！这就是 **`**vListInsertEnd**`** 的效果——不管值大小，硬插在 **`**pxIndex**`** 前面。**

---

## 第四步：列表在 FreeRTOS 实际项目中的应用

列表不是写来玩的数据结构小实验——**FreeRTOS 整个调度器就建立在列表上**。

### 4.1 就绪列表（Ready List）— 谁该运行

```c
// tasks.c 源码
static List_t pxReadyTasksLists[ configMAX_PRIORITIES ];  // 每个优先级一个列表！
```

```plain text
pxReadyTasksLists[0] → 优先级 0 的就绪任务链表
pxReadyTasksLists[1] → 优先级 1 的就绪任务链表
pxReadyTasksLists[2] → 优先级 2 的就绪任务链表
...
pxReadyTasksLists[31]→ 优先级 31 的就绪任务链表

每个任务创建时，其 TCB 里内嵌一个 ListItem_t（xStateListItem），
TCB 是 pvOwner。任务就绪 → 把它的列表项插入对应优先级链表。
调度器选最高优先级的非空链表，从中取一个任务运行。
```

### 4.2 延时列表（Delayed List）— 谁在睡觉

```c
static List_t xDelayedTaskList1;  // 延时的任务
static List_t xDelayedTaskList2;  // 溢出的延时任务
```

```plain text
你调 vTaskDelay(500)：
  → 当前 TCB 从就绪列表移除
  → 计算唤醒时间 = 当前 tick + 500
  → 把 TCB 的列表项按唤醒时间升序插入 xDelayedTaskList1

SysTick 每 1ms：
  → 检查 xDelayedTaskList1 头部（值最小的 = 最早该醒的）
  → 如果到时间了 → 移出延时列表 → 插回就绪列表
```

### 4.3 挂起列表（Suspended List）— 谁被暂停了

```c
static List_t xSuspendedTaskList;
```

```plain text
你调 vTaskSuspend(task1_handle)：
  → task1 的列表项从就绪/延时列表移除
  → 插入 xSuspendedTaskList

你调 vTaskResume(task1_handle)：
  → 从 xSuspendedTaskList 移回就绪列表
```

### 4.4 你项目的完整运行图

```plain text
任务的一生 = 在不同列表之间搬家：

              创建
               │
               ▼
        ┌──────────────┐      vTaskDelay()
        │  就绪列表      │ ──────────────────▶  ┌──────────────┐
        │ (按优先级分桶)  │                     │  延时列表      │
        └──────────────┘  ◀────────────────── └──────────────┘
               │            SysTick 到期唤醒        │
               │                                   │
        vTaskSuspend()                       vTaskSuspend()
               │                                   │
               ▼                                   ▼
        ┌──────────────────────────────────────────┐
        │           挂起列表                        │
        └──────────────────────────────────────────┘
               │
        vTaskResume() / xTaskResumeFromISR()
               │
               ▼
        ┌──────────────┐
        │  就绪列表      │
        └──────────────┘
               │
        vTaskDelete()
               │
               ▼
            释放内存
```

**这就是 FreeRTOS 调度器的本质——一堆链表，把任务 TCB 在不同链表中移来移去。**

### 4.5 另一个应用：你终端每 1ms 收到的"列表为空"提示

你在这个实验的串口输出可能见过类似效果——`vListInsert` 内部调了临界段，`uxListRemove` 以后 `uxNumberOfItems` 递减。FreeRTOS 每个滴答都遍历延时列表，找有没有任务该醒了。

---

## 第五步：关键 API 速查

| API | 作用 | 排序依据 |
| --- | --- | --- |
| `vListInitialise()` | 初始化列表（哨兵自环，计数归零） | — |
| `vListInitialiseItem()` | 初始化列表项（pxContainer = NULL） | — |
| `vListInsert()` | **按值升序插入** | `xItemValue` 值小在前 |
| `vListInsertEnd()` | **插在 pxIndex 前面** | 不管值，看游标位置 |
| `uxListRemove()` | 从链表中摘掉自己 | — |

---

## 一句话总结

> **FreeRTOS 的调度器 = 一堆环形双向链表。任务创建 → 链表节点插入就绪列表。任务延时 → 节点搬到延时列表。任务挂起 → 节点搬到挂起列表。SysTick 时扫描延时列表，到时间的搬回就绪列表。你在 **`**freertos_dome.c**`** 里手动操作的那些 API，就是内核每秒执行成千上万次的核心动作。**