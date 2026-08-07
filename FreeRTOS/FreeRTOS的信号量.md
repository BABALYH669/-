[[FreeRTOS队列]]
[[FreeRTOS队列集]]
# 信号量简介
信号量是一种解决问题的机制,可以实现对共享资源的有序访问
判断信号量是否有资源->信号量有资源(计数值)->获取信号量成功(计数值--)
判断信号量是否有资源->信号量没有资源->获取信号量失败
判断信号量是否有资源->信号量没有资源->任务阻塞->释放信号量资源(计数值++)

信号量的计数值都有限制,最大值为1->二值信号量,最大值不为1->计数型信号量

| 队列                                    | 信号量                                      |
| ------------------------------------- | ---------------------------------------- |
| 可以容纳多个数据；<br>创建队列有两部分内存：队列结构体+队列项存储空间 | 仅存放计数值，无法存放其他数据；<br><br>创建信号量，只需分配信号量结构体 |
| 写入队列：当队列满时，可阻塞；                       | 释放信号量：不可阻塞，计数值++，<br><br>当计数值为最大值时，返回失败  |
| 读取队列：当队列为空时，可阻塞；                      | 获取信号量：计数值--，<br><br>当没有资源时，可阻塞           |

# 二值信号量
本质是一个队列长度为1的队列,只有空(0)和满(1)两种情况

二值信号量通常用于互斥访问或任务同步， 与互斥信号量比较类似，
但是二值信号量有可能会导致优先级翻转的问题 ，所以二值信号量更适合用于同步！
![[wps_DosTsnOt0j.png]]
## 二值信号量API函数
创建二值信号量->释放二值信号量->获取二值信号量

| 函数                             | 描述            |
| ------------------------------ | ------------- |
| xSemaphoreCreateBinary()       | 使用动态方式创建二值信号量 |
| xSemaphoreCreateBinaryStatic() | 使用静态方式创建二值信号量 |
| xSemaphoreGive()               | 释放信号量         |
| xSemaphoreTake()               | 获取信号量         |
| xSemaphoreGiveFromISR()        | 在中断中释放信号量     |
| xSemaphoreTakeFromISR()        | 在中断中获取信号量     |
### 创建二值信号量函数
```
创建二值信号量函数
SemaphoreHandle_t   xSemaphoreCreateBinary( void ) 

 #define   xSemaphoreCreateBinary( )
 xQueueGenericCreate( 1 ,emSEMAPHORE_QUEUE_ITEM_LENGTH  ,queueQUEUE_TYPE_BINARY_SEMAPHORE )
 
 #define  semSEMAPHORE_QUEUE_ITEM_LENGTH (( uint8_t )0U)
 
 返回值      描述
 NULL       创建失败
 其他值      创建成功,返回二值信号量的句柄(SemaphoreHandle_t)

可选参数:
#define queueQUEUE_TYPE_BASE  ( ( uint8_t ) 0U )	
/* 队列 */
#define queueQUEUE_TYPE_SET   ( ( uint8_t ) 0U )	
/* 队列集 */
#define queueQUEUE_TYPE_MUTEX  ( ( uint8_t ) 1U )	
/* 互斥信号量 */
#define queueQUEUE_TYPE_COUNTING_SEMAPHORE (( uint8_t)2U)	/* 计数型信号量 */
#define queueQUEUE_TYPE_BINARY_SEMAPHORE (( uint8_t )3U)	/* 二值信号量 */
#define queueQUEUE_TYPE_RECURSIVE_MUTEX (( uint8_t )4U)	/* 递归互斥信号量 */
```
### 释放二值信号量函数
```
释放二值信号量函数：BaseType_t   xSemaphoreGive( xSemaphore ) 

#define   xSemaphoreGive (xSemaphore)
	xQueueGenericSend( (QueueHandle_t)(xSemaphore),NULL  ,semGIVE_BLOCK_TIME, queueSEND_TO_BACK )


#define   semGIVE_BLOCK_TIME  ( ( TickType_t ) 0U )

形参:xSemaphore,描述:要释放的信号量句柄
返回值              描述
pdPASS             释放信号量成功
errQUEUE_FULL      释放信号量失败

返回值类型是BaseType_t
BaseType_t本质是long类型,freeRTOS为了规范/移植统一使用BaseType_t
```

### 获取二值信号量
```
获取二值信号量函数：BaseType_t   xSemaphoreTake( xSemaphore, xBlockTime ) 

形参          描述
xSemaphore   要获取的信号量句柄
xBlockTime   阻塞时间 0~portMAX_DELAY

返回值           描述
pdTRUE          获取信号量成功
pdFALSE         超时，获取信号量失败
```

# 计数型信号量
计数型信号量相当于队列长度大于1的队列，创建时确定可容纳多个资源。
适用场合:
	事件计数: 每次事件后，处理函数释放计数信号量（+1），其他任
务获取（-1），初始值通常为0。
	资源管理:信号量表示可用资源数。任务先获取信号量（计数值-1）获得控制权，计数值为0时无资源可用。使用完后释放（计数值+1）。创建时计数值应等于最大资源数。
## 计数型信号量相关API函数
创建计数型信号量 -> 释放信号量 -> 获取信号量

| 函数                               | 描述              |
| -------------------------------- | --------------- |
| xSemaphoreCreateCounting()       | 使用动态方法创建计数型信号量。 |
| xSemaphoreCreateCountingStatic() | 使用静态方法创建计数型信号量  |
| uxSemaphoreGetCount()            | 获取信号量的计数值       |
### 计数型信号量创建API函数
```
#define (uxMaxCount ,uxInitialCount )       xQueueCreateCountingSemaphore((uxMaxCount),(uxInitialCount)) 

形参             描述
uxMaxCount      计数值的最大值限定
uxInitialCount  计数值的初始值

返回值           描述
NULL            创建失败
其他值           创建成功返回计数型信号量的句柄
```
### 获取信号量的计数值
```
#define uxSemaphoreGetCount( xSemaphore ) 				            uxQueueMessagesWaiting( ( QueueHandle_t ) ( xSemaphore ) )

形参           描述
xSemaphore    信号量句柄

返回值           描述
整数            当前信号量的计数值大小
```
# 优先级翻转
高优先级发任务反而慢执行,低优先级的任务反而先执行
常发生在二值信号量
![[wps_cTQAE0Fhl2.png]]
任务L先拿到了信号量。然后高优先级任务H抢占了L，但因为信号量还在L手上，H被阻塞了。接着任务M来了，它的优先级比L高，所以M开始执行。M不需要信号量，不用等待L释放。M执行完后，L才能继续。L执行完并释放信号量后，H才能拿到信号量并执行。

# 互斥信号量
一个拥有<font color="#ff0000">优先级继承</font>的二值信号量,适用于需要互斥访问的应用中
优先级继承：如果一个低优先级任务正拿着互斥信号量，这时一个高优先级任务也想拿这个信号量，高优先级任务就会被卡住。不过，<font color="#ff0000">高优先级任务会把低优先级任务的优先级提到和自己一样高</font>。
优先级继承不能完全解决优先级翻转的问题，只能尽量降低它带来的影响
![[wps_DLkVkzTw3R.png]]
此时任务H的阻塞时间仅仅是任务L 的执行时间，将优先级翻转的危害降到了最低

互斥信号量不能用于中断服务函数中，原因如下:
1. 互斥信号量有任务优先级继承的机制 ,但是中断不是任务，没有任务优先级,所以互斥信号量只能用与任务中,不能用于中断服务函数。
2. 中断服务函数中不能因为要等待互斥信号量而设置阻塞时间进入阻塞态。
## 互斥信号量相关API函数
使用互斥信号量,首先将宏configUSE_MUTEXES置一
创建互斥信号量->(task)获取信号量->(give)释放信号量
因为创建互斥信号量函数内部主动调用了一次释放信号量,所以创建后是先获取信号量,二值信号量是先释放再获取

| 函数                                 | 描述                 |
| ---------------------------------- | ------------------ |
| xSemaphoreCreateMutex() <br>       | 使用动态方法创建互斥信号量。<br> |
| xSemaphoreCreateMutexStatic() <br> | 使用静态方法创建互斥信号量。<br> |
### 互斥信号量创建API函数
```
#define xSemaphoreCreateMutex() 
         xQueueCreateMutex(queueQUEUE_TYPE_MUTEX )

返回值                  描述
NULL                   创建失败
其他值                  创建成功返回互斥信号量的句柄

```