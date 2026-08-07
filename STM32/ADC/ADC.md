---
notion-id: 35d5d378-8ef5-8012-9582-db96e89a71eb
---
[[ADC寄存器]]

## ADC(Analog-to-Digital Converter),模拟/数字转换器

传感器转为为电压经过ADC转换成数字量供单片机处理

| ADC电路类型 | 优点 | 缺点 |
| --- | --- | --- |
| 并联比较型 | 转换速度最快 | 成本高,功耗高,分辨率低 |
| 逐次逼近型 | 结构简单,功耗低 | 转换速度慢 |

ADC的特性参数

分辨率(刻度划分):表示ADC能辨别的最小模拟量,用二进制位数表示
转换时间:表示完成一次A/D转换所需时间,转换时间越短,采样率就越高
精度(物理量的精准程度):最小刻度基础上叠加各种误差的参数,精度收ADC性能,温度和气压等影响
量化误差:用数字量近似表示模拟量,采用四舍五入原则,此过程的误差就成为量化误差

![[TapTapLoot_hqBc36sN9C.png]]

### 转换序列

规则组:最多可有16个转换

注入组:最多可有4个转换,注入组可以打断规则组

![[TapTapLoot_gCgUkZDZZ8.png]]

![[TapTapLoot_lEjhbc4Lkb.png]]

### 触发源(两种)

1. ADON位触发转换
当ADC_CR2寄存器的ADON位为1时,再单独(只给ADON位写1)给ADON位写1,只能启动规则组转换
2. 外部事件触发转换
    1. 规则组外部触发
![[TapTapLoot_cF6ruTvQ1N.png]]
    2. 注入组外部触发
![[TapTapLoot_1XUW3zo4kc.png]]

### 转换时间

3. 设置ADC时钟
            PCLK2          →          ADCPRE[1:0]               →ADCCLK
(APB2总线上的时钟)         RCC_CFGR       ADC 最大时钟频率位14MHz                                    寄存器(分频系数2/4/6/8)
![[TapTapLoot_4y7gCKOn9b.png]]
4. 设置ADC准换时间
![[TapTapLoot_KFpnuuemQe.png]]

### 数据寄存器

分辨率是12位的,寄存器是16位的,涉及到了数据对齐,一般来说使用右对齐

![[TapTapLoot_ydJavuISOS.png]]

### 中断

![[TapTapLoot_mBlMXI3L81.png]]

### 单次转换模式和连续转换模式

![[TapTapLoot_8PvUcKBQSR.png]]

### 扫描模式

关闭扫描模式,如果设置了多个转换通道,只会转换第一个通道

![[TapTapLoot_RXqXsmQyuI.png]]

### 不同模式组合的作用

![[TapTapLoot_ouhN0aUnFd.png]]

## 单通道ADC采集实验(单次转换模式、不使用扫描模式)

### 单通道ADC采集实验配置步骤

![[wps_TzbTOgkXRH.png]]

### 相关HAL库函数介绍

![[wps_srT9hKjvoo.png]]

### **关键结构体**

```c
typedef struct 
{ 
ADC_TypeDef *Instance; 			/* ADC 寄存器基地址 */ 	
ADC_InitTypeDef Init; 				/* ADC 参数初始化结构体变量 */ 	
DMA_HandleTypeDef *DMA_Handle; 	/* DMA 配置结构体 */	
…… 
} ADC_HandleTypeDef;

typedef struct 
{ 	
uint32_t DataAlign; 					/* 设置数据的对齐方式 */ 	
uint32_t ScanConvMode; 				/* 扫描模式 */ 	
FunctionalState ContinuousConvMode; 	/* 开启单次转换模式或者连续转换模式 */ 	
uint32_t NbrOfConversion; 				/* 设置转换通道数目 */ 	
FunctionalState DiscontinuousConvMode; 	/* 是否使用规则通道组间断模式 */ 	
uint32_t NbrOfDiscConversion; 			/* 配置间断模式的规则通道个数 */ 	
uint32_t ExternalTrigConv; 				/* ADC 外部触发源选择 */ 
} ADC_InitTypeDef;

typedef struct { 	
uint32_t Channel; 			/* ADC 转换通道*/ 	
uint32_t Rank; 			/* ADC 转换顺序 */ 	
uint32_t SamplingTime; 	/* ADC 采样周期 */ 
}  ADC_ChannelConfTypeDef;
```

### ADC单通道采集总结

[[ACD单通道采集总结]]

## 单通道ADC采集(DMA读取)(循环转换模式、不使用扫描模式)

### 单通道ADC采集实验配置步骤

![[wps_k0MseFvzYw.png]]

### 相关HAL库函数介绍

![[wps_H5P4Ll82U7.png]]

### **关键结构体(DMA)**

```c
typedef struct 
{ 	
uint32_t Direction; 				   /* 传输方向 */ 	
uint32_t PeriphInc; 				   /* 外设（非）增量模式 */ 	
uint32_t MemInc; 				       /* 存储器（非）增量模式 */ 	
uint32_t PeriphDataAlignment;  /* 外设数据宽度 */ 	
uint32_t MemDataAlignment; 		 /* 存储器数据宽度 */ 	
uint32_t Mode; 					       /* 操作模式 */ 	
uint32_t Priority; 				     /* DMA通道优先级 */ 
}DMA_InitTypeDef
```

### 单通道ADC采集(DMA读取)总结

[[ADC 单通道（DMA 读取）总结]]

## 多通道ADC采集(DMA读取)(循环转换模式、使用扫描模式)

### 多通道ADC采集(DMA读取)总结

[[多通道ADC采集(DMA读取)总结]]

## 内部温度传感器实验

[[adc 1.c]]

[[adc 1.h]]

[[main 1.c]]

## 单通道ADC过采样实验

增加的分辨率位数计算过采样频率方程

![[wps_ya2xmD8xGF.png]]

提高n位分辨率就是4的n次方 ,最高测量精度是16位,也就是4的4次方 256倍
提高N位分辨率,需要右移N位,12→16位,要右移4位
2^12 = 4096 根据公式要提升4位要乘上 4^4 = 256
4096*256=1048576  1048576右移4位 换算就是2^16 = 65536

与单通道ADC实验不同的是

```c
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/ADC/adc.h"


/* ADC过采样技术, 是利用ADC多次采集的方式, 来提高ADC精度, 采样速度每提高4倍
 * 采样精度提高 1bit, 同时, ADC采样速度降低4倍, 如提高4bit精度, 需要256次采集
 * 才能得出1次数据, 相当于ADC速度慢了256倍. 理论上只要ADC足够快, 我们可以无限
 * 提高ADC精度, 但实际上ADC并不是无限快的, 而且由于ADC性能限制, 并不是位数无限
 * 提高结果就越好, 需要根据自己的实际需求和ADC的实际性能来权衡.
 */
 这里是与单通道ADC实验不同,要乘上256 把12位变成了16位
#define ADC_OVERSAMPLE_TIMES    256                         /* ADC过采样次数, 这里提高4bit分辨率, 需要256倍采样 */
#define ADC_DMA_BUF_SIZE        ADC_OVERSAMPLE_TIMES * 10   /* ADC DMA采集 BUF大小, 应等于过采样次数的整数倍 */

uint16_t g_adc_dma_buf[ADC_DMA_BUF_SIZE];                   /* ADC DMA BUF */

extern uint8_t g_adc_dma_sta;                               /* DMA传输状态标志, 0,未完成; 1, 已完成 */

int main(void)
{
    uint16_t i;
    uint32_t adcx;
    uint32_t sum;
    float temp;

    HAL_Init();                                 /* 初始化HAL库 */
    sys_stm32_clock_init(RCC_PLL_MUL9);         /* 设置时钟, 72Mhz */
    delay_init(72);                             /* 延时初始化 */
    usart_init(115200);                         /* 串口初始化为115200 */
    led_init();                                 /* 初始化LED */
    lcd_init();                                 /* 初始化LCD */

    adc_dma_init((uint32_t)&g_adc_dma_buf);     /* 初始化ADC DMA采集 */

    lcd_show_string(30,  50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30,  70, 200, 16, 16, "ADC OverSample TEST", RED);
    lcd_show_string(30,  90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 110, 200, 16, 16, "ADC1_CH1_VAL:", BLUE);
    lcd_show_string(30, 130, 200, 16, 16, "ADC1_CH1_VOL:0.000V", BLUE); /* 先在固定位置显示小数点 */

    adc_dma_enable(ADC_DMA_BUF_SIZE);           /* 启动ADC DMA采集 */

    while (1)
    {
        if (g_adc_dma_sta == 1)
        {
            /* 计算DMA 采集到的ADC数据的平均值 */
            sum = 0;

            for (i = 0; i < ADC_DMA_BUF_SIZE; i++)   /* 累加 */
            {
                sum += g_adc_dma_buf[i];
            }
						
            adcx = sum / (ADC_DMA_BUF_SIZE / ADC_OVERSAMPLE_TIMES); /* 取平均值 */
            右移4位,变成65536
            adcx >>= 4;   /* 除以2^4倍, 得到12+4位 ADC精度值, 注意: 提高 N bit精度, 需要 >> N */

            /* 显示结果 */
            lcd_show_xnum(134, 110, adcx, 5, 16, 0, BLUE);      /* 显示ADC采样后的原始值 */
						这里要除以65536
            temp = (float)adcx * (3.3 / 65536);                 /* 获取计算后的带小数的实际电压值，比如3.1111 */
            adcx = temp;                                        /* 赋值整数部分给adcx变量，因为adcx为u16整形 */
            lcd_show_xnum(134, 130, adcx, 1, 16, 0, BLUE);      /* 显示电压值的整数部分，3.1111的话，这里就是显示3 */

            temp -= adcx;                                       /* 把已经显示的整数部分去掉，留下小数部分，比如3.1111-3=0.1111 */
            temp *= 1000;                                       /* 小数部分乘以1000，例如：0.1111就转换为111.1，相当于保留三位小数。 */
            lcd_show_xnum(150, 130, temp, 3, 16, 0X80, BLUE);   /* 显示小数部分（前面转换为了整形显示），这里显示的就是111. */

            g_adc_dma_sta = 0;                                  /* 清除DMA采集完成状态标志 */
            adc_dma_enable(ADC_DMA_BUF_SIZE);                   /* 启动下一次ADC DMA采集 */
        }

        LED0_TOGGLE();
        delay_ms(100);
    }
}
在adc.c文件中 要把SamplingTime采样周期调成1.5周期,最小的周期
```

[[adc 2.c]]

[[adc 2.h]]

[[main 2.c]]

## 光敏传感器

光敏二极管:核心是一个PN结,对光强非常敏感,单向导电性,                                       工作时需加反向电压
暗电流:无光照时,反向电流很小,称为暗电流
光电流:有光照时,光的强度越大,反向电流也越大,形成光电流
 串联一个电阻,就可以得到电压的变化,再用ADC读取,就知道光强变化

[[adc 3.c]]

[[adc3.h]]

[[main 3.c]]
