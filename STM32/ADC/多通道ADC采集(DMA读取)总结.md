---
notion-id: 3615d378-8ef5-800a-a458-c667bb6e779b
---
## 一、实验整体回顾

**目标**
用 ADC1 同时采集 6 个模拟通道（PA1~PA6 对应 CH0~CH5），通过 DMA 把数据自动搬到内存 `g_adc_dma_buf[300]` 中，每次采集 50 轮，每轮 6 个数据。主循环里对每个通道的 50 个值取平均，换算成电压，在 LCD 上显示。

**文件分工**

- `adc.c`：ADC + DMA 初始化、启动、MSP 时钟配置、DMA 中断服务。
- `main.c`：调用初始化，检测完成标志，计算并显示结果。

**关键技术点**

- 扫描模式 + 连续转换模式
- DMA 普通模式（不循环）
- ADC 时钟分频与采样时间配合
- 手动重载 DMA 传输个数，实现可控批次采集

---

## 二、adc.c 重点难点详解

下面我们逐行解析 `adc.c`，把 HAL 函数背后对应的 **寄存器操作** 彻底搞清楚。

### 2.1 `adc_nch_dma_init` —— 顶层初始化

```c
void adc_nch_dma_init(uint32_t mar)
{
    ADC_ChannelConfTypeDef adc_ch_conf;

    __HAL_RCC_DMA1_CLK_ENABLE();        // 开启 DMA1 时钟
```

`__HAL_RCC_DMA1_CLK_ENABLE()` 会设置 `RCC->AHBENR` 的位 0（DMA1EN）为 1。

```c
    g_dma_nch_adc_handle.Instance = DMA1_Channel1;
    g_dma_nch_adc_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
    g_dma_nch_adc_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    g_dma_nch_adc_handle.Init.MemInc = DMA_MINC_ENABLE;
    g_dma_nch_adc_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    g_dma_nch_adc_handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    g_dma_nch_adc_handle.Init.Mode = DMA_NORMAL;
    g_dma_nch_adc_handle.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&g_dma_nch_adc_handle);
```

**HAL_DMA_Init 做了什么？**
它会把传入的结构体参数写入 `DMA1_Channel1` 的配置寄存器，重点如下：

| 结构体字段 | 目标寄存器 / 位 | 含义 |
| --- | --- | --- |
| `Direction` | `CCR` 的 `DIR` 位 (bit4) | 1 = 外设到存储器 |
| `PeriphInc` | `CCR` 的 `PINC` 位 (bit6) | 0 = 外设地址不递增 |
| `MemInc` | `CCR` 的 `MINC` 位 (bit7) | 1 = 存储器地址递增 |
| `PeriphDataAlignment` | `CCR` 的 `PSIZE` 位 (bit8-9) | 半字 (16位) |
| `MemDataAlignment` | `CCR` 的 `MSIZE` 位 (bit10-11) | 半字 (16位) |
| `Mode` | `CCR` 的 `CIRC` 位 (bit5) | 0 = 普通模式 |
| `Priority` | `CCR` 的 `PL` 位 (bit12-13) | 高优先级 |

这些设置确保每次从固定的 `ADC1->DR` 地址读取 16 位数据，存入递增的内存数组。

```c
    __HAL_LINKDMA(&g_adc_nch_dma_handle, DMA_Handle, g_dma_nch_adc_handle);
```

这句把 DMA 句柄和 ADC 句柄“绑定”，HAL 内部会通过它查找 DMA。

```c
    g_adc_nch_dma_handle.Instance = ADC1;
    g_adc_nch_dma_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    g_adc_nch_dma_handle.Init.ScanConvMode = ADC_SCAN_ENABLE;
    g_adc_nch_dma_handle.Init.ContinuousConvMode = ENABLE;
    g_adc_nch_dma_handle.Init.NbrOfConversion = 6;
    ...
    HAL_ADC_Init(&g_adc_nch_dma_handle);
```

**HAL_ADC_Init 对应的寄存器操作**：

- `ScanConvMode = ENABLE` → `ADC1->CR1` 的 `SCAN` 位（bit8）置 1。
- `ContinuousConvMode = ENABLE` → `ADC1->CR2` 的 `CONT` 位（bit1）置 1。
- `NbrOfConversion = 6` → `ADC1->SQR1` 的 `L[3:0]` 位写入 5（表示 6 个常规转换）。
- `ExternalTrigConv = ADC_SOFTWARE_START` → `ADC1->CR2` 的 `EXTSEL[2:0]` 全 0，表示软件触发。

这样 ADC 就被配置为：**扫描 6 个通道→连续转换→软件触发**。

```c
    HAL_ADCEx_Calibration_Start(&g_adc_nch_dma_handle);
```

校准过程会设置 `ADC1->CR2` 的 `CAL` 位，等待硬件自校准完成，消除内部偏移误差。

```c
    adc_ch_conf.Channel = ADC_CHANNEL_0;   // 通道号
    adc_ch_conf.Rank = ADC_REGULAR_RANK_1; // 转换顺序
    adc_ch_conf.SamplingTime = ADC_SAMPLETIME_239CYCLES_5; // 采样时间 239.5 周期
    HAL_ADC_ConfigChannel(...);
    // 依次配置 CH1...CH5
```

每一次 `HAL_ADC_ConfigChannel` 会：

- 根据 `Rank` 将通道编号写入 `ADC1->SQR3` 或 `SQR2` 或 `SQR1` 的 `SQx[4:0]` 字段（因为 6 个通道都在 SQR3 和 SQR2 里）。
- 根据 `SamplingTime` 设置 `ADC1->SMPR2` 中对应通道的 `SMPx[2:0]` 位（CH0~9 在 SMPR2，CH10~17 在 SMPR1）。这里 CH0~5 都在 `SMPR2` 中，采样时间设置为 239.5 个 ADC 时钟周期。

**为什么选 239.5 周期？** 保证输入阻抗大的情况下也能正常采样，同时 ADC 时钟为 12MHz（72MHz/6），一个转换时间约 (239.5+12.5) 周期 ≈ 21μs，完全满足精度要求。

```c
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 3, 3);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
```

使能 DMA1 通道 1 中断，并设置抢占和响应优先级均为 3。

```c
    HAL_DMA_Start_IT(&g_dma_nch_adc_handle, (uint32_t)&ADC1->DR, mar, 0);
    HAL_ADC_Start_DMA(&g_adc_nch_dma_handle, &mar, 0);
```

这两句预先启动 DMA 和外设，但**传输数量为 0**（暂不启动实际搬运），目的是让外设状态机就绪。真正的启动在 `adc_dma_enable()` 中完成。

---

### 2.2 `HAL_ADC_MspInit` —— 外设级初始化

```c
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1) {
        GPIO_InitTypeDef gpio_init_struct;
        RCC_PeriphCLKInitTypeDef adc_clk_init = {0};

        __HAL_RCC_ADC1_CLK_ENABLE();       // 使能 ADC1 时钟(RCC->APB2ENR bit9)
        __HAL_RCC_GPIOA_CLK_ENABLE();      // 使能 GPIOA 时钟

        gpio_init_struct.Pin = GPIO_PIN_1|...|GPIO_PIN_6;
        gpio_init_struct.Mode = GPIO_MODE_ANALOG;  // 模拟输入
        HAL_GPIO_Init(GPIOA,&gpio_init_struct);

        adc_clk_init.PeriphClockSelection = RCC_PERIPHCLK_ADC;
        adc_clk_init.AdcClockSelection = RCC_ADCPCLK2_DIV6; // PCLK2/6 = 72/6 = 12MHz
        HAL_RCCEx_PeriphCLKConfig(&adc_clk_init);
    }
}
```

- `RCC_ADCPCLK2_DIV6` 会配置 `RCC->CFGR` 的 `ADCPRE[1:0]` 位为 10b（6 分频），得到 12MHz 的 ADC 时钟。F103 要求 ADC 时钟不超过 14MHz，12MHz 很合理。
- GPIO 模拟模式是将引脚配置为浮空输入，内部上拉下拉均无效，确保输入阻抗最高。

---

### 2.3 `adc_dma_enable` —— 手动启动核心

```c
void adc_dma_enable(uint16_t cndtr)
{
    ADC1->CR2 &= ~(1 << 0);             // 关闭 ADC (ADON=0)
    DMA1_Channel1->CCR &= ~(1 << 0);    // 关闭 DMA 通道 (EN=0)
    while (DMA1_Channel1->CCR & (1 << 0)); // 等待硬件确认关闭
    DMA1_Channel1->CNDTR = cndtr;       // 设置传输数量 = 300
    DMA1_Channel1->CCR |= 1 << 0;       // 再次使能 DMA 通道 (EN=1)
    ADC1->CR2 |= 1 << 0;                // 再打开 ADC (ADON=1)
    ADC1->CR2 |= 1 << 22;               // 触发软件开始转换 (SWSTART=1)
}
```

**为什么需要这一步？**

- HAL 启动时没有指定真正的传输数量（给的是 0），我们在需要一批数据时才载入 `CNDTR` 并启动。
- 按照 STM32 手册要求，**修改 DMA 的 CNDTR 必须在通道关闭状态下进行**。所以这里先禁止 DMA 通道（`CCR` bit0 清零），加载计数值，再使能。
- ADC 的 ADON 位关闭/再打开可以复位 ADC 的内部状态，保证跟在 DMA 之后给出全新一次扫描。
- `SWSTART`（bit22）写 1 后，硬件立即启动一次常规转换序列（因为配置了软件触发和连续模式，转换完一轮后会自动开始下一轮，直到 DMA 把 300 个数据传输完毕）。

**寄存器细节回顾**`ADC1->CR2` 关键位：

- bit0 ADON：ADC 使能
- bit1 CONT：连续转换
- bit8 DMA：DMA 使能（由 HAL 在 `Start_DMA` 里设置）
- bit22 SWSTART：软件启动转换

`DMA1_Channel1->CCR` 关键位：

- bit0 EN：通道使能
- bit1 TCIE：传输完成中断使能（我们在 HAL_DMA_Start_IT 中已置位）
- bit4 DIR、bit5 CIRC、bit6 PINC、bit7 MINC 等已由 HAL_DMA_Init 设定。

---

### 2.4 DMA 中断服务函数

```c
void DMA1_Channel1_IRQHandler(void)
{
    if(DMA1->ISR & (1<<1))          // TCIF1 传输完成标志
    {
        g_adc_dma_sta = 1;          // 通知主循环：数据已就绪
        DMA1->IFCR |= 1<<1;         // 写 1 清除标志
    }
}
```

- `DMA1->ISR` 的 bit1 是通道 1 的传输完成中断标志（TCIF1），DMA 搬运完 300 个数据后硬件置 1。
- 清除标志必须向 `DMA1->IFCR` 的对应位写 1。
- 主循环检测到 `g_adc_dma_sta == 1` 后，处理数据，再调用 `adc_dma_enable` 启动下一轮。

---

## 三、主循环配合要点（main.c 中相关逻辑）

```c
while (1) {
    if (g_adc_dma_sta == 1) {
        // 对 6 个通道取平均
        // 显示原始值和电压
        g_adc_dma_sta = 0;               // 清标志
        adc_dma_enable(ADC_DMA_BUF_SIZE); // 重新启动下一轮采集
    }
    LED0_TOGGLE();
    delay_ms(100);
}
```

使用 **DMA_NORMAL** 模式而非循环模式，是为了精确控制每批数据的处理时机，避免在处理时 DMA 又更新了数据。

---

## 四、实验难点总结

1. **时钟树理解**
ADC 时钟来自 PCLK2 分频，72MHz/6=12MHz，填写到 `ADCPRE` 位。配置错误会导致 ADC 超频或转换异常。
2. **扫描模式与多通道顺序**`NbrOfConversion=6` 和 6 个 `ConfigChannel` 按照 Rank1~6 依次设置，决定了数据按什么顺序出现在 `ADC1->DR` 中。DMA 搬走的数据顺序就是 CH0→CH1→…→CH5 循环。
3. **DMA 传输控制**
    - 必须保证 DMA 每次搬半个字（16 位），因为 ADC 转换结果是 12 位右对齐，存放在 DR 的低 16 位。
    - 修改 `CNDTR` 必须先关 DMA 通道，否则可能无效。
4. **手动重载与中断配合**
采用普通模式，手动加载传输计数，中断通知主循环，这样数据采集和处理的时序非常清晰，适合需要稳定周期的应用。
5. **寄存器级操作带来的灵活性**
通过直接操作 `CR2`、`CCR` 等寄存器，我们能绕开 HAL 的一些固有流程（例如每次启动都必须重新初始化），实现更高效的自定义采集.

---