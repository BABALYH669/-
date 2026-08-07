[[FreeRTOS的信号量]]
[[FreeRTOS队列]]
# 队列集
队列:任务间传递的消息为同一种类型
队列集:能在任务间传递不同数据类型的消息
作用:对各队列或信号量进行"监听",无论哪个消息到来,都可以让任务退出阻塞状态
```
接收任务() 
{
	等待队列集消息;
	if(队列/信号量)
	{
		......
	}
}

```

队列在被添加到队列集之前，队列中不能有有效的消息 
# 相关的API函数

| 函数                           | 描述                 |
| ---------------------------- | ------------------ |
| xQueueCreateSet()            | 创建队列集              |
| xQueueAddToSet()             | 队列添加到队列集中          |
| xQueueRemoveFromSet()        | 从队列集中移除队列          |
| xQueueSelectFromSet()        | 获取队列集中有有效消息的队列     |
| xQueueSelectFromSetFromISR() | 在中断中获取队列集中有有效消息的队列 |

## 创建队列集
```
QueueSetHandle_t     xQueueCreateSet( const  UBaseType_t   uxEventQueueLength ); 

形参                  描述
uxEventQueueLength   队列集可容纳的队列数量

返回值                描述
NULL                 队列集创建失败
其他值                队列集创建成功，返回队列集句柄 
```

## 队列添加到队列集中
```
BaseType_t xQueueAddToSet( QueueSetMemberHandle_t   	xQueueOrSemaphore ,QueueSetHandle_t	xQueueSet  	); 

形参                 描述
xQueueOrSemaphore  待添加的队列句柄
xQueueSet          队列集

返回值描述
pdPASS队列集添加队列成功
pdFAIL队列集添加队列失败
```

## 队列集中移除队列
```
BaseType_t xQueueRemoveFromSet( QueueSetMemberHandle_t 	xQueueOrSemaphore ,QueueSetHandle_t xQueueSet ); 


形参描述
xQueueOrSemaphore待移除的队列句柄
xQueueSet队列集

返回值描述
pdPASS队列集移除队列成功
pdFAIL队列集移除队列失败
```

## 在任务中获取队列集中有有效消息的队列
```
QueueSetMemberHandle_t xQueueSelectFromSet(QueueSetHandle_t  xQueueSet,TickType_tconst  xTicksToWait)

形参描述
xQueueSet队列集
xTicksToWait阻塞超时时间

返回值描述
NULL获取消息失败
其他值获取到消息的队列句柄
```

### 使用流程
1. 将宏configUSE_QUEUE_SETS置一
2. 创建队列集
3. 创建队列或信号量
4. 往队列集中添加队列或信号量
5. 往队列发送信息或释放信号量
6. 获取队列集消息