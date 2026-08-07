# 事件标志组简介
用一个位,来表示事件是否发生
用一个字节(八位) 0000 0000 一个事件对应一个位,事件发生就把对应的位置一,然后再while死循环里判断
事件标志组:一组事件标志位的集合,就是一个整数
特点:
1. 每一个位表示一个事件(高八位不算)
2. 每一件事件的含义,由用户自己决定(位,置一表示发生,置零代表未发生)
3. 任意任务或中孤单都可以读写这些位
4. 可以等待某一位成立,或者等待多位同时成立(两种等待方式)
一个事件组就包含了一个``EventBites_t``数量类型(16位或32位)
```
typedef TickType_t EventBits_t;
#if ( configUSE_16_BIT_TICKS  ==  1 )
	typedef  uint16_t  TickType_t;
#else
	typedef  uint32_t  TickType_t;
#endif
#define  configUSE_16_BIT_TICKS    0
```
32 位无符号的数据类型变量来存储事件标志， 其中高8位用作存储事件标志组的控制信息，低24位用作存储事件标志 ，一个事件组<font color="#ff0000">最多可以存储 24 个事件标志</font>![[wps_HWr4BLMbTz.png]]
事件标志组与队列、信号量的区别

| 功能     | 唤醒对象                            | 事件清除                            |
| ------ | ------------------------------- | ------------------------------- |
| 队列、信号量 | 事件发生时，只会唤醒一个任务                  | 是消耗型的资源，队列的数据被读走就没了；信号量被获取后就减少了 |
| 事件标志组  | 事件发生时，会唤醒所有符合条件的任务，可以理解为“广播”的作用 | 被唤醒的任务有两个选择，可以让事件保留不动，也可以清除事件   |
# 事件标志组相关API函数介绍

| 函数                            | 描述               |
| ----------------------------- | ---------------- |
| xEventGroupCreate()           | 使用动态方式创建事件标志组    |
| xEventGroupCreateStatic()     | 使用静态方式创建事件标志组    |
| xEventGroupClearBits()        | 清零事件标志位          |
| xEventGroupSetBits()          | 设置事件标志位          |
| xEventGroupWaitBits()         | 等待事件标志位          |
| xEventGroupSync()             | 设置事件标志位，并等待事件标志位 |
| xEventGroupClearBitsFromISR() | 在中断中清零事件标志位      |
| xEventGroupSetBitsFromISR()   | 在中断中设置事件标志位      |
## 动态方式创建事件标志组API函数

```
EventGroupHandle_t    xEventGroupCreate ( void ) ; 
返回值          描述
NULL           事件标志组创建失败
其他值          事件标志组创建成功,返回其句柄
```
## 清除事件标志位API函数
```
EventBits_t  xEventGroupClearBits( 
						EventGroupHandle_t 	xEventGroup,                            const EventBits_t 	uxBitsToClear )
 
形参               描述
xEventGroup       待操作的事件标志组句柄
uxBitsToClear     待清零的事件标志位

返回值             描述
整数               清零事件标志位之前事件组中事件标志位的值
```
## 设置事件标志位API函数
```
EventBits_t   xEventGroupSetBits(  
					EventGroupHandle_t 	xEventGroup,					        const EventBits_t 	uxBitsToSet   ) 

形参               描述
xEventGroup       待操作的事件标志组句柄
uxBitsToClear     待清零的事件标志位
有三种方式:
1.0x01 
2.1 << 0 
3.定义一个宏

返回值             描述
整数               函数返回时，事件组中的事件标志位值
```
## 等待事件标志位API函数
```
EventBits_t xEventGroupWaitBits( 
			 EventGroupHandle_t 	xEventGroup,
 			 const EventBits_t 	    uxBitsToWaitFor,
 			 const BaseType_t 	    xClearOnExit,
 			 const BaseType_t 	    xWaitForAllBits,
 			 TickType_t 		    xTicksToWait )
 			 
形参                描述
xEvenrGroup        等待的事件标志组句柄
uxBitsToWaitFor    等待的事件标志位，可以用逻辑或等待多个事件标志                     位
xClearOnExit       成功等待到事件标志位后，清除事件组中对应的事件                     标志位，
                   pdTRUE :清除uxBitsToWaitFor指定位
                   pdFALSE:不清除
xWaitForAllBits   等待uxBitsToWaitFor中的所有事件标志位(逻辑与)                    pdTRUE：等待的位，全部为1 
                   pdFALSE：等待的位，某个为1
xTicksToWait       等待的阻塞时间


返回值                描述
等待的事件标志位值      等待事件标志位成功，返回等待到的事件标志位
其他值                等待事件标志位失败，返回事件组中的事件标志位 

特点
 1.可以等待某一位,也可以等待多位
 2.等到期望的事件后，还可以清除某些位
```
## 同步函数
```
EventBits_txEventGroupSync(
						EventGroupHandle_t   xEventGroup,
						const EventBits_t   uxBitsToSet,
						const EventBits_t   uxBitsToWaitFor,
						TickType_t          xTicksToWait  )

形参                  描述
xEventGroup          等待事件标志所在事件组
uxBitsToSet          达到同步点后，要设置的事件标志
uxBitsToWaitFor      等待的事件标志
xTicksToWait         等待的阻塞时间


返回值                 描述
等待的事件标志位值      等待事件标志位成功，返回等待到的事件标志位
其他值                等待事件标志位失败，返回事件组中的事件标志位

```
