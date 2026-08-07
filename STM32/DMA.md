---
notion-id: 3595d378-8ef5-8036-bd9f-f7b4344d7f88
---
## DMA介绍

1. 全程Direct Memory Access,即直接寄存器访问
2. DMA传输,将数据从一个地址空间复制到另一个地址空间,地址=外设(DR)=内存
3. 无需CPU直接控制传输,通过硬件为RAM和IO设备开辟通道,使CPU效率提高
4. 作用:大量数据传输的时候为CPU减负.

![[TapTapLoot_i8jNEigFnI.png]]

## DMA寄存器

### DMA通道x配置寄存器(DMA_CCRx)

![[TapTapLoot_vRPqJaQ8Od.png]]

### DMA中断状态寄存器(DMA_ISR)

![[TapTapLoot_OO2wmjl89O.png]]

### DMA中断标志清除寄存器(DMA_IFCR)

![[TapTapLoot_nJ1D7CylyA.png]]

### DMA通道x传输数量寄存器(DAM_CNDTR)

![[TapTapLoot_tdChqXMVUf.png]]

### DMA通道x外设地址寄存器(DMA_CPARx)
DAM通道x存储器地址寄存器(DMA_CMARx)

![[TapTapLoot_lLOxR2FugI.png]]

### DMA相关HAL库驱动

![[TapTapLoot_OXErewIh8O.png]]

### DMA外设相关结构体

```c
typedef struct 
{     
DMA_Channel_TypeDef	*Instance
DMA_InitTypeDef 		Init    
}DMA_HandleTypeDef;
```

```c
typedef struct 
{     
uint32_t Direction			/* DMA传输方向 */
uint32_t PeriphInc			/* 外设地址(非)增量 */
uint32_t MemInc			/* 存储器地址(非)增量*/
uint32_t PeriphDataAlignment	/* 外设数据宽度 */
uint32_t MemDataAlignment	/* 存储器数据宽度 */
uint32_t Mode				/* 操作模式 */
uint32_t Priority				/* DMA通道优先级 */   
}DMA_InitTypeDef;
```

### 配置步骤

![[TapTapLoot_15c7Uy0NaM.png]]

![[wps_4wNj2pqK7E.png]]
