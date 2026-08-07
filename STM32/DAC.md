---
notion-id: 3645d378-8ef5-8008-8b8a-e16d33e3661f
---
## DAC(Digital-to-Analog Converter),数字模拟转换器

ADC是将传感器的电信号转化为数字量让单片机识别,DAC是将单片机识别后的数字转变为电信号,输出给模拟控制系统

### DAC的特性参数

![[wps_HpkPGzsayQ.png]]

比例系统误差:   斜率偏差
失调误差:           斜率相同,起点不同
非线性误差:       直线弯曲

![[wps_JHg0MY8Hr4.png]]

### DAC框图

![[wps_aqyQRvS6wl.png]]

### DAC数据格式

![[wps_oxLUtkzuPz.png]]

### 触发源

![[wps_MZW9QdJL2W.png]]

自动:经过一个DAC周期自动把DHRx中写入的数据量传到DORx中
软件:触发源中的SWTRIGx软件触发事件产生,把DHRx中写入的数据           量传到DORx中
外部事件:定时器和EXTI_9硬件产生事件,DHRx中写入的数据量传到                   DORx中

### DMA请求

![[wps_bXl2BLPif0.png]]

### DAC输出电压

![[wps_qocRNB5v26.png]]

## DAC输出实验

![[wps_J4z2aA0y3T.png]]

自动促发把TEN1位置0,关闭DAC1通道触发
TEN位(位2)是 DAC通道1触发使能
EN1位(位0)是 DAC通道1使能
关闭输出缓冲,是因为使能的话最低伏测不到0伏

### DAC输出实验配置步骤

![[wps_IZCJR7tOlw.png]]

![[wps_t6FGT5pUUo.png]]

```c
typedef struct 
{ 	
DAC_TypeDef *Instance; 			 /* DAC 寄存器基地址 */ 	
__IO HAL_DAC_StateTypeDef State; 	/* DAC 工作状态 */ 	
HAL_LockTypeDef Lock; 			/* DAC 锁定对象 */ 	
DMA_HandleTypeDef *DMA_Handle1; 	/* 通道 1 的 DMA 处理句柄指针 */ 	
DMA_HandleTypeDef *DMA_Handle2; 	/* 通道 2 的 DMA 处理句柄指针 */ 	
__IO uint32_t ErrorCode; 			/* DAC 错误代码 */ 
} DAC_HandleTypeDef; 

typedef struct 
{ 	
uint32_t DAC_Trigger; 		/* DAC 触发源的选择 */ 	
uint32_t DAC_OutputBuffer; 	/* 启用或者禁用 DAC 通道输出缓冲区 */ 
} DAC_ChannelConfTypeDef;
```

## DAC输出三角波实验

### 实验简要

![[wps_tfM2bYJjto.png]]

![[wps_iP7SQeE3f5.png]]

## DAC输出正弦波实验

### 实验简介

![[wps_LXrW1kKwiS.png]]

### 配置步骤

![[wps_gtilbpt0Ph.png]]

### 产生正弦波函数序列

![[wps_enM5urtjCa.png]]