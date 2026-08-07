---
notion-id: 36c5d378-8ef5-80d7-994e-e0b1839f6f16
---
## 介绍

SPI:串行外设设备接口(Serial Perpheral Interface),是一种高速的,全双工,同步的通信总线 

![[wps_B5qtxbgKTr.png]]

CS是片选,代表着低电平有效
数据格式和传输顺序都靠从机选择
SPI是边沿协议
IIC是电平协议

## SPI工作模式

![[wps_9B1TwdHQ84.png]]

![[wps_Vjv8J0QywD.png]]

### SPI相关寄存器

![[wps_cUkPMYiiSF.png]]

![[wps_xaisqCDAF5.png]]

![[wps_0chjA9wTOZ.png]]

![[wps_IhVnjxYq5L.png]]

![[wps_24o9ZUMsjI.png]]

![[wps_TLsf6E7WkB.png]]

```javascript
typedef struct
{
  uint32_t Mode;        /* 设置SPI模式（主机模式） */
  uint32_t Direction;   /* 设置SPI工作方式（全双工） */       
  uint32_t DataSize;    /* 设置数据格式（8bit长度） */        
  uint32_t CLKPolarity; /* 设置时钟极性（CPOL = 1） */        
  uint32_t CLKPhase;     /* 设置时钟相位（CPHA = 1） */       
  uint32_t NSS;          /* 设置片选方式（软件片选，自定义GPIO） */       
  uint32_t BaudRatePrescaler; /* 设置SPI时钟波特率分频（256分频） */
  uint32_t FirstBit;   /* 设置大小端模式（MSB高位在前） */         
  uint32_t TIMode;     /* 设置帧格式（关闭TI模式） */        
  uint32_t CRCCalculation; /* 设置CRC校验（关闭CRC校验） */     
  uint32_t CRCPolynomial;  /* 设置CRC校验多项式（范围：1~65535） */        
} SPI_InitTypeDef;
```

## NOR FLASH

FLASH是常用的用于储存数据的半导体器件,它具有容量大,可重复擦写,按”扇区/块” 擦除,掉电后数据可继续保存的特性

FLASH物理特性:只能写0,不能写1,写1靠擦除

物理上只能对整个块同时施加高压电场——无法精确到单个 bit。所以：**写 0 可以逐位，写 1 必须整块擦除。**

| **维度** | **NOR Flash** | **NAND Flash** |
| --- | --- | --- |
| **读取粒度** | 随机字节访问 | 页读取（通常 2KB/4KB/16KB） |
| **读延迟** | 低（~70-100ns） | 高（~25-50μs 首字节延迟） |
| **写入粒度** | 字节/字 | 页（page）编程 |
| **擦除粒度** | 大块（64KB-256KB） | 块（更大，通常 MB 级） |
| **擦除速度** | 慢（~0.5-1s/块） | 快（~2-5ms/块） |
| **写入吞吐** | 低 | 高（支持多 plane 并发） |
| **容量** | 小（MB 级） | 大（GB-TB 级） |
| **成本/bit** | 高 | 低 |
| **擦写寿命** | ~100K 次 | ~3K-100K 次（取决于 SLC/MLC/TLC/QLC） |
| **XIP 支持** | ✅ 支持 | ❌ 不支持 |
| **典型用途** | 固件/BIOS/Bootloader | 大容量数据存储（SSD、U 盘、SD 卡） |

## NM25Q128

串行闪存器件，属于NOR FLASH中的一种，容量为128 Mb。擦写周期可达10W次，可以将数据保存达20年之久

SPI数据传输时序:支持模式0（CPOL = 0 , CPHA = 0）和模式3（CPOL = 1, CPHA = 1）

数据格式:数据长度8位大小,先发高位,再发低位

传输速度:支持标准模式104Mbit/s

![[wps_72cjo03XQi.png]]

25Q128:
256个块,
一块 = 16扇区 
一扇区 = 16 页 
一页= 256字节

![[wps_f0c38pnrUk.png]]

### 常用指令

![[wps_9sinNM5E4b.png]]

SI是主机输出线
SO是主机接收线(从机输出线)


![[wps_vpKhldtEi0.png]]

05h/35h/15h 分别对应 读寄存器SR1/SR2/SR3

![[wps_YOmbdaxYx5.png]]

发送0x03,代表这读数据
读指令后面的地址代表着要读取地址上的数据
一个字节为单位 24个字节分三次发送,Data Out 返回数据

![[wps_Rc1aeRpa1s.png]]

发送0x02代表着要写入数据
写指令后面的地址代表着要写入数据的地址
写超过256个字节的数据要考虑换页

![[wps_YvIKBIlyg7.png]]

![[wps_R9MqvqdfJR.png]]

![[wps_0X2KbsiwwN.png]]

![[wps_KAPImEIB5Z.png]]

![[wps_2SI8iZUdfh.png]]

![[wps_eDmj1fbH4n.png]]

## NOR FLASH基本驱动步骤

![[wps_tBgwTybvV1.png]]

![[wps_G9tUbLBPuM.png]]

读出数据存放到大数组中→改写大数组中的数据→擦除扇区→把读取的数据+修改的数据再写入扇区

128Mb = 16MB = 16777216字节,2^24 = 16777216,所以要发送24位地址,分为高八位,中八位,低八位发送,对16MB内存精确寻址0x000000~0xFFFFFF