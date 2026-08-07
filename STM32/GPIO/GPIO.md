---
notion-id: 33a5d378-8ef5-80b5-a83e-c05ccbb364c9
---
## 什么是GPIO?

General Purpose Input Output , 通用输入输出端口,简称GPIO

作用,负责采集外部器件的信息或者控制外部期间工作,即输入输出

![[msedge_TNj6oqybaJ.png]]

![[msedge_685w9o6mZq.png]]

## GPIO特点

1. 不同型号,IO口可能数量不一样,用选型手册查
2. 快速翻转,最快只需要两个周期 
3. 每个IO口都可以做中断
4. 支持8种工作模式

## GPIO电气特性

5. STM32工作电压范围:2V≤VDD≤3.6V
6. GPIO识别电压范围
    1. COMS端口:
     -0.3V≤Vil≤1.164V 判断为逻辑0   
     1.833V≤Vih≤3.6V 判断为逻辑1  
     在中间的范围时是不确定的可能为0可能为1
    2. TTL端口 3.3V≤TTL≤5V
7. GPIO输出电流:单个IO口最大25mA

## GPIO引脚分布

STM32引脚类型:

8. 电源引脚:以V字母开头的
9. 晶振引脚:P/OSC开头的
    - **OSC_IN / OSC_OUT**接 **HSE 外部高速晶振**（通常 8MHz）
    - **OSC32_IN / OSC32_OUT**接 **LSE 32.768KHz 晶振**（给 RTC 用）
10. 复位引脚:NRST
11. 下载引脚
    - **SWDIO** ➜ **PA13** (第 38 脚)
    - **SWCLK** ➜ **PA14** (第 39 脚)
    - **GND** ➜ **VSS** (任意地线，如第 63 脚)
12. BOOT引脚:BOOT0 BOOT1
13. GPIO引脚:P字母开头的

[[GPIO的八种工作模式]]

## GPIO使用方法总结

LED和KEY的

首先都是建立一个.c 和 .h文件

led.h文件                                                               

```c
#ifndef _LED_H
#define _LED_H
#include "./SYSTEM/sys/sys.h"
void led_init(void);
/*引脚定义*/
#define     LED0_GPIO_PORT   GPIOB
#define     LED0_GPIO_PIN    GPIO_PIN_5
//PB口时钟使能
//其中do while(0) 等价于 __HAL_RCC_GPIOB_CLK_ENABLE(); 只是套了一个壳子,表示只执行一次
#define LED0_GPIO_CLK_ENABLE()  do{__HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

//引脚定义
#define     LED1_GPIO_PORT  GPIOE
#define     LED1_GPIO_PIN   GPIO_PIN_5
//PE口时钟使能
#define LED1_GPIO_CLK_ENABLE()  do{__HAL_RCC_GPIOE_CLK_ENABLE();}while(0)
/*
    do while(0)里是一个三目运算符,其中'\'是宏定义中表示换行的
    以下两个参数是在第一行第二行定义了的
    LED0_GPIO_PORT
    LED0_GPIO_PIN
    以下的参数代表输出高电平  = 1
    GPIO_PIN_SET
*/
//LED端口定义
//这一段在主函数main.c文件中使用,x = 1 执行亮 x = 0执行灭
#define LED0(x) do{ x ? \
    HAL_GPIO_WritePin(LED0_GPIO_PORT,LED0_GPIO_PIN,GPIO_PIN_SET) :\
    HAL_GPIO_WritePin(LED0_GPIO_PORT,LED0_GPIO_PIN,GPIO_PIN_RESET);\
                   }while(0)

#define LED1(x) do{ x ? \
    HAL_GPIO_WritePin(LED1_GPIO_PORT,LED1_GPIO_PIN,GPIO_PIN_SET) :\
    HAL_GPIO_WritePin(LED1_GPIO_PORT,LED1_GPIO_PIN,GPIO_PIN_RESET);\
                   }while(0)

//LED取反定义
#define LED0_TOGGLE()  do{ HAL_GPIO_TogglePin(LED0_GPIO_PORT,LED0_GPIO_PIN);}while(0)
#define LED1_TOGGLE()  do{ HAL_GPIO_TogglePin(LED1_GPIO_PORT,LED1_GPIO_PIN);}while(0)
#endif

```

KEY.h文件

```c
#ifndef __KEY_H
#define __KEY_H

#include "./SYSTEM/sys/sys.h"
//函数声明
void key_init(void);
uint8_t key_scan01(void);
uint8_t key_scan00(void);
//引脚定义,key1的引脚是PE3
#define     KEY1_GPIO_PORT  GPIOE
#define     KEY1_GPIO_PIN   GPIO_PIN_3

#define     KEY0_GPIO_PORT  GPIOE
#define     KEY0_GPIO_PIN   GPIO_PIN_4
//PE口时钟使能
#define KEY_GPIO_CLK_ENABLE()   do{__HAL_RCC_GPIOE_CLK_ENABLE();}while(0)

#define KEY1 HAL_GPIO_ReadPin(KEY1_GPIO_PORT,KEY1_GPIO_PIN)
#define KEY0 HAL_GPIO_ReadPin(KEY0_GPIO_PORT,KEY0_GPIO_PIN)
#endif


```

关于GPIO在HAI库里一共有五个最常用函数

14. GPIO 初始化函数 `HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);`
    1. 配置 GPIO 引脚的工作模式 
    2. 
```javascript
gpio_init_struct.Pin = LED0_GPIO_PIN;//初始化引脚编号
gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;//推挽输出
gpio_init_struct.Pull = GPIO_PULLUP;//开启上拉电阻,提供高电平
gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH; //GPIO反转速度为高速
HAL_GPIO_Init(LED0_GPIO_PORT,&gpio_init_struct);//调用HAL库函数HAL_GPIO_Init
告诉HAL库 调用的是LED0_GPIO_PORT = GPIOB这个端口 
					gpio_init_struct是上面写的参数 Pin Mode Pull Speed
```
15. 写引脚电平（输出高 / 低）`HAL_GPIO_WritePin(GPIOx, GPIO_Pin,GPIO_PIN_SET/GPIO_PIN_RESET);`
16. 读引脚电平（输入状态）`HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);`
17. 翻转引脚电平 `HAL_GPIO_TogglePin(GPIOx, GPIO_Pin);`
18. GPIO 时钟使能（最重要！) `__HAL_RCC_GPIOx_CLK_ENABLE();`

```c
.h文件开头都是先写一个 #ifdef和#endif是配套的
#ifndef __文件名(大写)_H
#define __文件名(大写)_H
写引用的头文件,正点原子的sys.h文件

然后把.c文件中的函数声明写在头文件里

定义引脚
#define  元件名_GPIO_端口(PORT)  端口(GPIOx) x=A~G
#define  元件命_GPIO_引脚(PIN)   GPIO_引脚(PIN)_引脚数 = 0~15 

时钟使能 其中的GPIOx是想让哪个端口使能就写哪个端口
#define 元件名_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOx_CLK_ENABLE(); }while(0)
以下几个函数选择使用
其中
GPIO_TypeDef *GPIOx就是上面定义的端口 
uint16_t GPIO_Pin 引脚号
GPIO_PinState PinState 输出的状态分为
		GPIO_PIN_SET 表示高电平，
		GPIO_PIN_RESET 表示低电平。
HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin,
GPIO_PinState PinState);

HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);
#endif
```

```c
首先写头文件 
#include 文件.h

void 函数名(void){
    //定义GPIO初始化结构体变量,起名为gpio_init_struct
    GPIO_InitTypeDef gpio_init_struct;
    这个再头文件里定义
    元件名_GPIO_CLK_ENABLE();//使能元件   GPIO端口时钟
   
    
    gpio_init_struct.Pin = 元件名_GPIO_PIN;//初始化引脚编号
    gpio_init_struct.Mode = 见下图
    gpio_init_struct.Pull = 见下图
    gpio_init_struct.Speed = 见下图
    //调用HAL库函数HAL_GPIO_Init,初始化
    HAL_GPIO_Init(LED0_GPIO_PORT,&gpio_init_struct);

    
}


```

led.c文件                                                             

```c
//包含LED驱动的头文件
#include "./BSP/LED/LED.h"
//初始化led_init函数
void led_init(void){
    //定义GPIO初始化结构体变量,起名为gpio_init_struct
    GPIO_InitTypeDef gpio_init_struct;
    LED0_GPIO_CLK_ENABLE();//使能LED0   GPIO端口时钟
    LED1_GPIO_CLK_ENABLE();//使能LED1   GPIO端口时钟
    
    gpio_init_struct.Pin = LED0_GPIO_PIN;//初始化引脚编号
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;//推挽输出
    gpio_init_struct.Pull = GPIO_PULLUP;//开启上拉电阻,提供高电平
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH; //GPIO反转速度为高速
    HAL_GPIO_Init(LED0_GPIO_PORT,&gpio_init_struct);//调用HAL库函数HAL_GPIO_Init
    
    gpio_init_struct.Pin = LED1_GPIO_PIN;
    HAL_GPIO_Init(LED1_GPIO_PORT,&gpio_init_struct);
    
    LED0(1);
    LED1(1);
    这两句是用来 给 LED 一个初始状态，让上电后 LED 保持熄灭（或点亮），避免灯乱闪、状态不确定
    
}


```

 key.c文件

```c
#include "./BSP/KEY/KEY.h"
#include "./SYSTEM/delay/delay.h"//延时函数的头文件
void key_init(void)
{
    GPIO_InitTypeDef gpio_init_struct; //初始化GPIO结构体
    
    KEY_GPIO_CLK_ENABLE();//时钟使能
    
    gpio_init_struct.Pin = GPIO_PIN_3;//PE3引脚
    gpio_init_struct.Mode = GPIO_MODE_INPUT;//输入模式
    gpio_init_struct.Pull = GPIO_PULLUP;//上拉电阻
    
    HAL_GPIO_Init(GPIOE,&gpio_init_struct);

}

//按键消抖函数(扫描)
uint8_t key_scan01(void)
{
    if(KEY1 == 0) //检测按键是否按下(低电平有效)
    {
        delay_ms(10);//延迟10ms为了给按键消抖
        if(KEY1 == 0)//再次确认是否按下
        {
            //while(KEY1 == 0);//等待按键按下松开加上按下按键会有明显延迟
            delay_ms(10);//延迟10ms为了给按键消抖
            return 1;//确认按下返回1
        }
    }
    return 0;//无效按键返回0
}

//按键消抖函数(扫描)
uint8_t key_scan00(void)
{
    if(KEY0 == 0) //检测按键是否按下(低电平有效)
    {
        delay_ms(10);//延迟10ms为了给按键消抖
        if(KEY0 == 0)//再次确认是否按下
        {
            //while(KEY0 == 0);//等待按键按下松开
            delay_ms(10);//延迟10ms为了给按键消抖
            return 1;//确认按下返回1
        }
    }
    return 0;//无效按键返回0
}


```


![[LdSypdtKAt.png]]

```c
main.c 主函数内写的
前三个是必备的 是正点原子自己编写的简化开发的
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
下面两个是自己编写的头文件包含宏定义和函数声明等等
#include "./BSP/LED/LED.h"
#include "./BSP/KEY/KEY.h"
int main(void)
{
    HAL_Init();                                 /* 初始化HAL库 */
    sys_stm32_clock_init(RCC_PLL_MUL9);         /* 设置时钟,72M */
    delay_init(72);                             /* 初始化延时函数 */
    led_init();                                 /* 初始化LED */
    key_init();                                 /* 初始化KEY */
    while(1)
    {
        if(key_scan01())
        {
            int num = 10;
            while(num)
            {
                LED1(1);
                delay_ms(200);
                LED0(1);
                delay_ms(200);
                LED1(0);
                delay_ms(200);
                LED0(0);
                delay_ms(200);
                num--;
            }
            LED1(0);
            LED0(0);
        }
        else if(key_scan00())
        {
            LED1(1);
            LED0(1);
        }
    }
}


```

![[BongoCat_HymSo41aE4.png]]

KEY_UP和函数中的key_up重复了,会导致编译器分不清而报错