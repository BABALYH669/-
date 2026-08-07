---
notion-id: 3435d378-8ef5-80ae-b8cc-f3d243bc70ff
---
# IWDG

简介:Independent watchdog  即 独立看门狗

本质:产生系统复位信号的计数器

特性:递减的计数器,时钟由独立的RC振荡器提供,可在待机和停止模式下运行,看门狗激活后,当递减计数器计数到0x000时产生复位

喂狗:在计数器计数到0之前,重装载计数器的值

异常:陷入不正常的死循环,打断程序的正常运行

作用:检测外键电磁干扰,或者硬件异常导致的程序跑飞

应用:需要高稳定的产品中,对时间精度较低的要求

独立看门狗时异常处理的最后手段,不可依赖,在设计师尽量避免异常

工作原理:

![[TapTapLoot_fQ2zYTdKPR.png]]

### 系统复位(5个):

1. NRST引脚上的低电平(外部复位)
2. 窗口看门狗计数中止(WWDG复位)
3. 独立看门狗计数中止(IWDG复位)
4. 软件复位(SW复位)
5. 低功耗管理复位

### IWDG寄存器

1. 键寄存器(IWDG_KR):
    1. 一点间隔写入0xAAAA,防止计数器归零 (喂狗) 0xAAAA = 1010 1010 1010 1010
    2. 写入0x5555表示允许访问IWDG_PR和IWDG_RLR 寄存器 0x5555 = 0101 0101 0101 0101
        - 解除PR RLR寄存器写访问保护
    3. 写入0xCCCC,启动看门狗工作 0xCCCC = 1100 1100 1100 1100
2. 预分频寄存器(IWDG_PR)
    - 只有前三个位有效[2:0] 
    - 4 * 2^prer = PSC  4 * 2^分频因子 = 分频系数
![[TapTapLoot_GVZ7NpUT4I.png]]
3. 重装载寄存器(IWDG_RLR)
    5. 只有12个位有效[11:0]
    6. 用于定义看门狗计数器的重装载值
    7. 这里设定重装载值,当键寄存器写入0xAAAA时,把重装载值写入到计数器中
    8. 只有当IWDG_SR寄存器中的RVU位为零时,才能修改
4. 状态寄存器(IWDG_SR)
    9. 只有两位有效[1:0]
    10. 位1,看门狗计数器重装载值更新,更新置1,更新结束后由硬件清0
    11. 位0,看门狗预分频值更新,,更新置1,更新结束后由硬件清0
    12. 可以判断是否完成

![[TapTapLoot_xhs6nLrDnC.png]]

以上寄存器的设置在HAL库中一个函数就可以都设置

### IWDG溢出时间计算

![[TapTapLoot_NADgSfWO64.png]]

![[TapTapLoot_fv3rhYnkpT.png]]

### IWDG配置步骤

![[TapTapLoot_YW4GWWDLBe.png]]

```c
#include "头文件"

IWDG_HandleTypeDef g_iwdg_handle;

IWDG初始化函数
uint8_t prer,uint16_t rlr			
设置为8位和16位是为了节省内存,原本是32位,但是有保留位
void iwdg_init(uint8_t prer,uint16_t rlr)
{
	g_iwdg_handle.Instance = IWDG;
	g_iwdg_handle.Init.Prescaler = 分频系数 prer
	g_iwdg_handle.Init.Reload = 重装载值 rlr
	HAL_IWDG_Init(句柄)
	
}

喂狗函数
void iwdg_feed(void)
{
	HAL_IWDG_Refresh(句柄);
}
```

```c
#ifndef __WDG_H
#define __WDG_H

#include "./SYSTEM/sys/sys.h"


void iwdg_init(uint8_t prer, uint16_t rlr);
void iwdg_feed(void);

#endif
```

```c
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/WDG/wdg.h"


int main(void)
{
    HAL_Init();                             /* 初始化HAL库 */
    sys_stm32_clock_init(RCC_PLL_MUL9);     /* 设置时钟为72Mhz */
    delay_init(72);                         /* 延时初始化 */
    usart_init(115200);                     /* 串口初始化为115200 */
    
    printf("您还没喂狗，请及时喂狗！！！\r\n");
    iwdg_init(IWDG_PRESCALER_32, 1250);     /* 预分频系数为32，重装载值为1250，溢出时间约为1s */
    while (1)
    {
        delay_ms(1000);
        iwdg_feed();
        printf("已经喂狗\r\n");
    }
}
```

# WWDG

全称:Window watchdog,窗口看门狗

本质: 能产生系统复位信号和提前唤醒中断的计数器

特质:递减计数器,从0x40(64)减到0x3F(63)时会复位→即T6位跳变到0
          0x40 = 0100 0000          0x3F = 0011 1111
          计数器的值大于W[6:0}时喂狗会复位
          提前唤醒中断(EWI):当递减计数器等于0x40时可产生

喂狗:在窗口期内重装载计数器的值,防止复位

作用:用于检测单片机程序运行时效是否精准,主要检测软件异常

应用:需要精准检测程序运行时间的场合

![[TapTapLoot_Rg9FxLmb0i.png]]

![[TapTapLoot_6xUrztSNue.png]]

控制寄存器(WWDG_CR)

配置寄存器(WWDG_CFR)

位9:EWI 提前唤醒中断 同时对应的NVIC中断也要使能
             置1后,只要计数器达到0x40,就会产生中断

位[8:7} WDGTB[1:0] 定时器时基→ 2^WDGTB[1:0]

位6 W[6:0] 七位窗口上限值,用于和T[6:0]进行比较,高于 W[6:0] 喂狗会复位,                      低于 W[6:0] 喂狗不会复位

状态寄存器(WWDG_SR)

WWDG超时时间计算公式

![[TapTapLoot_NXcMqPie1p.png]]

**超时时间 =（固定时钟分频计数周期）×（计数衰减跨越阈值所需的周期数）**

T[5:0]+1  寄存器是7个位最大值是127(0x7F),到了63(0x3F)会复位,插值是64,而T[5:0]最大值是63所以要加1

### 配置步骤

![[TapTapLoot_mJQ4TR0t6Y.png]]

```c
#include "头文件"

WWDG_HandleTypeDef g_wwdg_handle;

/* 窗口看门狗初始化函数 */
void wwdg_init(uint8_t tr, uint8_t wr, uint32_t fprer)
{
		
    g_wwdg_handle.Instance = WWDG;寄存器基地址
    g_wwdg_handle.Init.Counter = tr;计数器的初始值
    g_wwdg_handle.Init.Window = wr; 窗口值
    g_wwdg_handle.Init.Prescaler = fprer; 分频系数
    g_wwdg_handle.Init.EWIMode = WWDG_EWI_ENABLE; 产生唤醒中断使能
    HAL_WWDG_Init(&g_wwdg_handle);
}

/* WWDG MSP回调函数 */
void HAL_WWDG_MspInit(WWDG_HandleTypeDef *hwwdg)
{
    __HAL_RCC_WWDG_CLK_ENABLE(); 使能时钟
    
    HAL_NVIC_SetPriority(中断号,抢占优先级,响应优先级);
    HAL_NVIC_SetPriority(WWDG_IRQn, 2, 3);
    
    HAL_NVIC_EnableIRQ(中断号);使能中断
}

/* WWDG中断服务函数 */
void WWDG_IRQHandler(void)
{
    HAL_WWDG_IRQHandler(&g_wwdg_handle);HAL库的公共处理函数
}

/* WWDG提前唤醒回调函数 */
void HAL_WWDG_EarlyWakeupCallback(WWDG_HandleTypeDef *hwwdg)
{
    HAL_WWDG_Refresh(&g_wwdg_handle);
    LED1_TOGGLE();
}
```

```c
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led.h"
#include "./BSP/WDG/wdg.h"


int main(void)
{
    HAL_Init();                                 /* 初始化HAL库 */
    sys_stm32_clock_init(RCC_PLL_MUL9);         /* 设置时钟,72M */
    delay_init(72);                             /* 初始化延时函数 */
    usart_init(115200);                         /* 波特率设置为115200 */
    led_init();                                 /* 初始化LED */
    
    __HAL_RCC_GET_FLAG标志检查函数
    RCC_FLAG_WWDGRST 看门狗复位标志 等于0就是复位
    __HAL_RCC_CLEAR_RESET_FLAGS() 置0
    if(__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != RESET)
    {
        printf("窗口看门狗复位\r\n");
        __HAL_RCC_CLEAR_RESET_FLAGS();
    }
    else
    {
        printf("外部复位\r\n");
    }
    
    delay_ms(500);
    printf("请在窗口期内喂狗\r\n\r\n");
    wwdg_init(0x7f, 0x5f, WWDG_PRESCALER_8);
    
    while(1)
    {
        delay_ms(90);
        HAL_WWDG_Refresh(&g_wwdg_handle);
        LED0_TOGGLE();
    }
}
```

### IWDG和WWDG实验总结

[[IWDG和WWDG实验总结]]
