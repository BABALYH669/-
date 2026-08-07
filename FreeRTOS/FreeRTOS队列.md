---
notion-id: 3955d378-8ef5-8046-9244-ffa3508431b5
---
[[FreeRTOS队列集]]
[[FreeRTOS的信号量]]

## 队列简介

队列是任务到任务,任务到中断,中断到任务数据交流的一种机制(消息传递)

全局变量的弊端:数据无保护,导致数据不安全,多个任务同时对该变量操作时,数据易受损
使用队列:读写队列中添加了**临界区保护,不会打断任务执行**,所以可以完整执行任务,防止了多任务同时访问冲突,只需要调用API函数即可,简单易用

队列:可以存储数量有限,大小固定的数据
         数据:”队列项目”,存储数据的最大数量:”队列长度”
创建队列时要规定队列长度和项目的大小

关键特性
特性	        说明
FIFO	        先进先出，保证数据顺序
线程安全	多任务同时读写，内核自动保护
阻塞机制	队列空/满时任务可阻塞等待
数据拷贝	数据按值传递（复制进队列），而非指针
固定大小	创建时确定每个元素的大小和队列长度

FreeRTOS队列特点
1. 数据入队出队方式:通常”先进先出”也支持配置”后进先出(LIFO)”
2. 数据传递方式:实际值传递(数据拷贝到队列中进行传递),传递较大数据时采用指针传递
3. 多任务访问:任何任务和中断都可以想队列发送/读取消息
4. 出队入队阻塞:向一个队列发送消息时,可以指定一个阻塞时间
	1. 若阻塞时间为0:直接返回不会等待
	2. 若阻塞时间为0~portMAX_DELAY:等待设定的阻塞时间,若在该时间内还无法入队,超时后直接返回不再等待
	3. 若阻塞时间为portMAX_DELAY:死等,一直等到可以入队为止
	4. 出队阻塞和入队阻塞类似![[wps_03bvpYsnNe.png]]
    5. 多个任务写入一个"满队列"时,这些任务都处于阻塞状态,若队列空出一个位置,1:给优先级最高的任务,2优先级相同给等待时间最长的任务
  队列就是一个线程安全的 memcpy 中转站——你把完整数据拷进去，另一边把完整数据拷出来。中间不管发生什么中断、切换，拷贝进去的数据纹丝不动。
  ```
  typedef struct QueueDefinition 
{
    int8_t * pcHead;			/* 存储区域的起始地址 */
    int8_t * pcWriteTo;     /* 下一个写入的位置 */
    union
    {
	    QueuePointers_t     xQueue; 
		SemaphoreData_t  xSemaphore; 
    } u ;
    List_t xTasksWaitingToSend; 	/* 等待发送列表 */
    List_t xTasksWaitingToReceive;	/* 等待接收列表 */
    volatile UBaseType_t uxMessagesWaiting; 	
                             /* 非空闲队列项目的数量 */
    UBaseType_t uxLength;			/* 队列长度 */
    UBaseType_t uxItemSize;      /* 队列项目的大小 */
    volatile int8_t cRxLock;     /* 读取上锁计数器 */
    volatile int8_t cTxLock;	 /* 写入上锁计数器 */
   /* 其他的一些条件编译 */
} xQUEUE;


union
    {
	    QueuePointers_t     xQueue; 
		SemaphoreData_t  xSemaphore; 
    } u ;

当用于队列使用时
typedef struct QueuePointers
{
     int8_t * pcTail; 		/* 存储区的结束地址 */
     int8_t * pcReadFrom;	/* 最后一个读取队列的地址 */
} QueuePointers_t;

当用于互斥信号量和递归互斥信号量时:
typedef struct SemaphoreData
{
    TaskHandle_t xMutexHolder;	/* 互斥信号量持有者 */
    UBaseType_t uxRecursiveCallCount;	
    /* 递归互斥信号量的获取计数器 */
} SemaphoreData_t;
  ```
## 队列相关API函数
### 创建队列API函数
xQueueCreate()  动态方式创建->自动分配内存(从FreeRTOS管理的堆中分配)
```
#define xQueueCreate (uxQueueLength,uxItemSize)   			xQueueGenericCreate( (uxQueueLength), (uxItemSize), (queueQUEUE_TYPE_BASE )) 
uxQueueLength:队列长度
uxItemSize:队列项目的大小
返回值:
NULL-创建失败,
其他值-创建成功,返回队列句柄

queueQUEUE_TYPE_BASE 代表队列类型一下是参数
#define queueQUEUE_TYPE_BASE                  			   ( (uint8_t) 0U )	/* 队列 */
#define queueQUEUE_TYPE_SET                  			   ( (uint8_t) 0U )	/* 队列集 */
#define queueQUEUE_TYPE_MUTEX                 			   ( (uint8_t) 1U )	/* 互斥信号量 */
#define queueQUEUE_TYPE_COUNTING_SEMAPHORE    	            ( (uint8_t) 2U )	/* 计数型信号量 */
#define queueQUEUE_TYPE_BINARY_SEMAPHORE     	           ( (uint8_t) 3U )	/* 二值信号量 */
#define queueQUEUE_TYPE_RECURSIVE_MUTEX       		       ( (uint8_t) 4U )	/* 递归互斥信号量 */


```

xQueueCreateStatic()  静态方式创建队列->手动分配内存
## 写入队列

| 函数                         | 描述                         |
| -------------------------- | -------------------------- |
| xQueueSend()               | 往队列的尾部写入消息                 |
| xQueueSendToBack()         | 同 xQueueSend()             |
| xQueueSendToFront()        | 往队列的头部写入消息                 |
| xQueueOverwrite()          | 覆写队列消息（只用于队列长度为 1 的情况）     |
| xQueueSendFromISR()        | 在中断中往队列的尾部写入消息             |
| xQueueSendToBackFromISR()  | 同 xQueueSendFromISR()      |
| xQueueSendToFrontFromISR() | 在中断中往队列的头部写入消息             |
| xQueueOverwriteFromISR()   | 在中断中覆写队列消息（只用于队列长度为 1 的情况） |
其中前四个写入函数调用的是同一个函数xQueueGenericSend(),只是指定了不同的写入位置
一共有3个参数
```
#define queueSEND_TO_BACK  ( ( BaseType_t ) 0 )		
/* 写入队列尾部 */
#define queueSEND_TO_FRONT ( ( BaseType_t ) 1 )		
/* 写入队列头部 */
#define queueOVERWRITE     ( ( BaseType_t ) 2 )		
/* 覆写队列*/ 只用于队列长度为 1 的情况
```
xQueueSend = xQueueSendToBack

```
BaseType_t  xQueueGenericSend(  
					QueueHandle_t 	xQueue,			
					const void * const 	pvItemToQueue, 
					TickType_t 		xTicksToWait,
					const BaseType_t 	xCopyPosition );
xQueue:待写入的队列
pvItemToQueue:待写入的消息
xTicksToWait:阻塞超时时间
xCopyPosition:写入的位置
返回值
pdTRUE:队列写入成功
errQUEUE_FULL:队列写入失败
```
## 读取队列

| 函数                          | 描述                      |
| --------------------------- | ----------------------- |
| xQueueReceive() <br>        | 从队列头部读取消息，并删除消息<br>     |
| xQueuePeek() <br>           | 从队列头部读取消息<br>           |
| xQueueReceiveFromISR() <br> | 在中断中从队列头部读取消息，并删除消息<br> |
| xQueuePeekFromISR() <br>    | 在中断中从队列头部读取消息<br>       |
```
BaseType_t    xQueueReceive( 
			QueueHandle_t   xQueue,  
			void *   const pvBuffer,  
			TickType_t   xTicksToWait )

xQueue:待读取的队列
pvBuffer:信息读取缓冲区
xTicksToWait:阻塞超时时间
返回值
pdTRUE:读取成功
pdFALSE:读取失败
BaseType_t   xQueuePeek( 
			QueueHandle_t   xQueue,   
			void * const   pvBuffer,   
			TickType_t   xTicksToWait )

xQueue:待读取的队列
pvBuffer:信息读取缓冲区
xTicksToWait:阻塞超时时间
返回值
pdTRUE:读取成功
pdFALSE:读取失败
```
