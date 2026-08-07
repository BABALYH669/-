---
notion-id: 3615d378-8ef5-80e3-b0b4-c4ae6292f57a
---
## 📘 STM32F103ZET6 ADC 单通道（DMA 读取）实验笔记

### 一、实验目的

1. 掌握 **ADC 连续转换模式** 配合 **DMA 循环/单次传输** 的配置方法。
2. 理解 DMA 将 ADC 数据自动搬运到内存，**释放 CPU**，提高系统效率。
3. 学习 HAL 库中 `HAL_ADC_Start_DMA` 的使用及 DMA 中断处理流程。
4. 掌握对一组采样值进行**均值滤波**，提高测量稳定性。

### 二、实验硬件

- 正点原子 STM32F103ZET6 开发板
- **PA1** 作为模拟输入（ADC1 通道1）
- LCD 屏用于显示测量结果
- 不需要额外接线，内置电压或可调电阻即可测试

### 三、实验步骤（结合代码详解）

### 1. 系统初始化

```c
sys_stm32_clock_init(RCC_PLL_MUL9); // SYSCLK = 72MHz
delay_init(72);
usart_init(115200);
led_init();
lcd_init();
```

常规外设初始化，系统时钟 72MHz，APB2 也是 72MHz。

### 2. 配置 ADC 与 DMA 的基础资源：`adc_dma_init()` 函数

**① 使能 DMA1 时钟，初始化 DMA 通道**

```c
__HAL_RCC_DMA1_CLK_ENABLE();
g_dma_adc_handle.Instance = DMA1_Channel1;  // ADC1 专用 DMA1 通道1
g_dma_adc_handle.Init.Direction = DMA_PERIPH_TO_MEMORY; // 外设->内存
g_dma_adc_handle.Init.PeriphInc = DMA_PINC_DISABLE;     // 外设地址不变
g_dma_adc_handle.Init.MemInc = DMA_MINC_ENABLE;          // 内存地址递增
g_dma_adc_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
g_dma_adc_handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
g_dma_adc_handle.Init.Mode = DMA_NORMAL;                // 单次模式，传完指定数量后停止
g_dma_adc_handle.Init.Priority = DMA_PRIORITY_HIGH;
HAL_DMA_Init(&g_dma_adc_handle);
```

- **为什么外设地址不递增？** ADC 数据寄存器（ADC1->DR）只有一个地址，每次转换结果都从同一个地址读。
- **为什么内存地址递增？** 我们的缓冲区 `g_adc_dma_buf` 是一个数组，需要依次存放每个采样点。
- **传输宽度为半字**，因为 STM32F103 的 ADC 分辨率是 12 位，数据寄存器是 16 位。

**② 将 DMA 句柄链接到 ADC 句柄**

```c
__HAL_LINKDMA(&g_adc_dma_handle, DMA_Handle, g_dma_adc_handle);
```

这样才能让 HAL 库在启动 ADC DMA 时使用我们配置好的 DMA 通道。

**③ 初始化 ADC 为连续模式**

```c
g_adc_dma_handle.Instance = ADC1;
g_adc_dma_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
g_adc_dma_handle.Init.ScanConvMode = ADC_SCAN_DISABLE;      // 单通道
g_adc_dma_handle.Init.ContinuousConvMode = ENABLE;          // 连续转换模式
g_adc_dma_handle.Init.NbrOfConversion = 1;
g_adc_dma_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START; // 软件触发
HAL_ADC_Init(&g_adc_dma_handle);
```

- **关键区别：** `ContinuousConvMode = ENABLE`，ADC 一旦启动就会不断转换，每次转换结束触发一次 DMA 请求。
- 其他参数与单次模式一致。

**④ 校准 ADC 并配置通道**

```c
HAL_ADCEx_Calibration_Start(&g_adc_dma_handle);
adc_ch_conf.Channel = ADC_CHANNEL_1;
adc_ch_conf.Rank = ADC_REGULAR_RANK_1;
adc_ch_conf.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
HAL_ADC_ConfigChannel(&g_adc_dma_handle, &adc_ch_conf);
```

通道 1，采样时间 239.5 周期。

**⑤ 配置 DMA 中断并初始启动**

```c
HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 3, 3);
HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

HAL_DMA_Start_IT(&g_dma_adc_handle, (uint32_t)&ADC1->DR, (uint32_t)g_adc_dma_buf, 0);
HAL_ADC_Start_DMA(&g_adc_dma_handle, (uint32_t *)&g_adc_dma_buf, 0);
```

- 开启 DMA 传输完成中断，以便传输完指定数量数据后产生中断。
- 启动时 `CNDTR` 传 0，HAL 内部不会立即开始 DMA，后面由 `adc_dma_enable()` 重新设置传输数量并真正启动。

### 3. 重新启动 DMA 传输的函数：`adc_dma_enable(uint16_t cndtr)`

```c
void adc_dma_enable(uint16_t cndtr)
{
    ADC1->CR2 &= ~(1 << 0);      // ADON 位置 0，停止 ADC
    DMA1_Channel1->CCR &= ~(1 << 0); // 关闭 DMA 通道
    while (DMA1_Channel1->CCR & (1 << 0));
    DMA1_Channel1->CNDTR = cndtr;    // 设置本次要传输的数据数量
    DMA1_Channel1->CCR |= 1 << 0;    // 重新开启 DMA
    ADC1->CR2 |= 1 << 0;            // 重新开启 ADC
    ADC1->CR2 |= 1 << 22;           // 启动规则组转换（SWSTART）
}
```

- **为什么需要这个函数？** 使用 DMA_NORMAL 模式，一次传输完成（CNDTR 减到 0）后 DMA 自动停止，ADC 即使仍在连续转换也不再产生 DMA 请求。为了再次采集一批数据，必须**先关 ADC 和 DMA**，重装 DMA 计数器，再启动它们。
- 操作寄存器非常底层，保证顺序无误。

### 4. DMA 中断服务函数

```c
void DMA1_Channel1_IRQHandler(void)
{
    if(DMA1->ISR & (1<<1))   // 判断传输完成中断标志（TCIF1）
    {
        g_adc_dma_sta = 1;   // 通知主循环数据已就绪
        DMA1->IFCR |= 1<<1;  // 清除中断标志
    }
}
```

非常简单，只是置一个标志位，主函数负责后续处理。

### 5. MspInit 回调（系统级初始化）

与之前相同：使能 GPIOA、ADC1 时钟，配置 PA1 为模拟输入，ADC 时钟分频 6 → 12MHz。

### 6. 主函数处理流程

```c
uint16_t g_adc_dma_buf[ADC_DMA_BUF_SIZE];   // 100 个半字
extern uint8_t g_adc_dma_sta;

int main(void)
{
    // 初始化 ADC DMA并启动初始传输
    adc_dma_init((uint32_t)g_adc_dma_buf);
    adc_dma_enable(ADC_DMA_BUF_SIZE);   // 开始采集 100 个点

    while (1)
    {
        if(g_adc_dma_sta == 1)          // DMA传输100个数据完成
        {
            // 1. 对 100 个数据求平均值（均值滤波）
            sum = 0;
            for(i = 0; i < ADC_DMA_BUF_SIZE; i++)
                sum += g_adc_dma_buf[i];
            adcx = sum / ADC_DMA_BUF_SIZE;

            // 2. 显示原始均值
            lcd_show_xnum(134, 110, adcx, 4, 16, 0, BLUE);

            // 3. 计算电压并显示（保留三位小数）
            temp = (float)adcx * (3.3 / 4096);
            // ...整数部分、小数部分拆分显示

            g_adc_dma_sta = 0;                 // 清除标志
            adc_dma_enable(ADC_DMA_BUF_SIZE);  // 再次启动下一轮 100 个点采集
        }
        LED0_TOGGLE();
        delay_ms(100);
    }
}
```

**非常高效！** CPU 只在每采集完 100 个点后才花一点时间处理数据，其余时间可以执行其他任务或休眠，这也是 DMA 的核心优势。

---

### 四、实验结论

- 通过 **DMA_NORMAL + 连续 ADC**，成功实现了对单通道模拟信号的**批量高速采集**，CPU 负载极低。
- 100 次采样后求均值，有效滤除了随机噪声，电压显示稳定。
- 软件必须通过重载 CNDTR 并重启外设的方式实现循环采集，这在大量数据实时采集中是常用的基础方法。
- 硬件连接正确时，PA1 接 3.3V 显示 4095 / 3.300V；接 GND 显示 0 / 0.000V；中间电压线性良好。

---

### 五、**重点、难点、要点总结**

### 🔴 重点

5. **DMA 配置参数的含义**
    - 方向：外设到内存
    - 外设地址不增，内存地址递增
    - 数据宽度：半字
    - 模式：Normal（单次，用于一批采集） vs Circular（循环，后续会学）
6. **ADC 连续模式与 DMA 的配合**
    - ADC 必须开启 `ContinuousConvMode`，才能持续触发 DMA 请求。
    - 启动方式：`HAL_ADC_Start_DMA`，传入缓冲区地址和数据长度。
7. **DMA 完成中断的使用**
    - 在 `DMA1_Channel1_IRQHandler` 中置标志，通知主循环处理数据，不占用主循环时间。
8. **一轮采集结束后需重装 DMA 计数器**
    - 通过 `adc_dma_enable()` 函数关闭 ADC 和 DMA，重新设置 `CNDTR`，再启动。

### 🟡 难点

9. **DMA 启动时 **`**CNDTR=0**`** 的设计思想**
    - 初始化时 `HAL_ADC_Start_DMA` 及 `HAL_DMA_Start_IT` 的 `data length` 参数传 0，是为了只使能但不开始实际传输，留给后面控制。这为灵活控制每一批采集的数量提供了方便。
10. **寄存器级别的重启动操作**
    - 必须按顺序：关 ADC → 关 DMA → 等 DMA 完全关闭 → 重设 CNDTR → 开 DMA → 开 ADC → 触发转换。
    - 如果采用 CubeMX 生成的代码并用 Cube HAL 提供的 `HAL_ADC_Start_DMA` 重启，有时会因为状态机冲突而失败，所以这里直接寄存器操作更可靠。
11. **均值滤波的精度权衡**
    - 采样点数越多滤波效果越好，但响应变慢。100 点对于低频信号展示已经足够平滑。

### 🟢 要点

- **DMA 请求与 ADC 的联动**：当 ADC 每个规则通道转换完成后，如果 DMA 使能，ADC 会自动发出 DMA 请求，无需软件干预。
- **数据缓冲区大小**：`ADC_DMA_BUF_SIZE` 定义为 100，CNDTR 加载的值必须与之匹配。
- **中断优先级**：DMA1_Channel1 中断优先级设为 3,3，不能过高影响其他关键中断。
- **标志清除**：`g_adc_dma_sta` 必须在数据处理后清除，并调用 `adc_dma_enable` 开始新的传输，否则程序会一直处理还是老数据。
- **LCD 显示注意**：显示前先画好字符串框架，再用数字覆盖，防止乱码。

---

### 六、拓展思考

- 如果希望 DMA 不停止，一直覆盖缓冲区，可以使用 `DMA_CIRCULAR` 模式，这样 ADC 数据会循环写入缓冲区，无需每轮重装。但是此时得用“半满/全满”中断或双缓冲机制保证数据不丢。
- 多通道扫描模式下，DMA 会按顺序搬运每个通道的数据，实现多路同时采集，这是我们下一步要学的内容。

---

**总结：本实验使用 ADC 连续转换 + DMA Normal 模式，成功实现高效、稳定的单通道电压采集与均值滤波，深刻理解了 DMA 与 ADC 联动、重装机制以及中断驱动编程的优点，为后续复杂数据采集系统奠定了坚实基础。**