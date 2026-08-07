---
notion-id: 35f5d378-8ef5-803d-8b81-c43d61bb3691
---
## 📘 STM32F103ZET6 ADC 单通道实验笔记

### 一、实验目的

1. 掌握 STM32 ADC 的 **单通道单次转换** 配置方法。
2. 学会使用 **HAL 库** 进行 ADC 初始化、校准、启动和读取结果。
3. 理解 ADC 时钟配置、采样时间对转换结果的影响。
4. 学会将 ADC 原始值转换为实际电压值并在 LCD 上显示。

### 二、实验硬件

- 正点原子 STM32F103ZET6 开发板（或核心板）
- 使用 **PA1（ADC1通道1）** 作为模拟输入
- 可通过杜邦线将 PA1 连接到 3.3V、GND 或可调电阻，观察电压变化
- LCD 屏用于显示结果

---

### 三、实验步骤

5. **系统时钟初始化**
    - `sys_stm32_clock_init(RCC_PLL_MUL9);` 设置系统时钟 72MHz，APB2 总线时钟也是 72MHz。
6. **GPIO 时钟与 ADC 时钟配置**
    - 在 `HAL_ADC_MspInit` 中使能 `GPIOA` 和 `ADC1` 时钟。
    - 配置 `PA1` 为模拟输入模式 (`GPIO_MODE_ANALOG`)。
7. **ADC 时钟分频设置**
    - 通过 `HAL_RCCEx_PeriphCLKConfig` 设置 ADC 外设时钟来源：`RCC_ADCPCLK2_DIV6`。
    - 意味着 ADC 时钟 = 72MHz / 6 = **12MHz**。
    - **为什么选 12MHz？** 因为 STM32F103 的 ADC 最大推荐工作频率是 14MHz，12MHz 是稳定且常用的选择。
8. **ADC 工作模式初始化**
```c
g_adc_handle.Instance = ADC1;
g_adc_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;      // 数据右对齐
g_adc_handle.Init.ScanConvMode = ADC_SCAN_DISABLE;      // 非扫描模式（单通道）
g_adc_handle.Init.ContinuousConvMode = DISABLE;         // 单次转换模式
g_adc_handle.Init.NbrOfConversion = 1;                  // 只转换一个通道
g_adc_handle.Init.DiscontinuousConvMode = DISABLE;      // 不使用间断模式
g_adc_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START; // 软件触发启动
HAL_ADC_Init(&g_adc_handle);
```
> 这里的配置决定了 ADC 只转换一个通道，每次调用 `HAL_ADC_Start` 才转换一次，转换完自动停止。
9. **ADC 校准**
    - `HAL_ADCEx_Calibration_Start(&g_adc_handle);`
    - ADC 内部有个自校准功能，能消除电容阵列的误差，**每次上电后建议执行一次校准**，提高精度。
10. **配置通道参数**
```c
adc_ch_conf.Channel = ADC_CHANNEL_1;                  // ADC1 通道 1
adc_ch_conf.Rank = ADC_REGULAR_RANK_1;                // 规则组第1个转换
adc_ch_conf.SamplingTime = ADC_SAMPLETIME_239CYCLES_5; // 采样时间：239.5 个 ADC 时钟周期
HAL_ADC_ConfigChannel(&g_adc_handle, &adc_ch_conf);
```
    - **采样时间为什么选 239.5 周期？** 对于一般电压测量，采样时间长一些可以**提高稳定性**，但会降低转换速度。这里不需要高速采集，所以选最长的采样时间。
    - 总转换时间 ≈ 采样时间 + 12.5 个固定周期 = 239.5 + 12.5 = 252 个 ADC 时钟周期 ≈ 21µs（12MHz 时钟下）。
11. **主循环中获取 ADC 值**
```c
HAL_ADC_Start(&g_adc_handle);                    // 软件触发启动一次转换
HAL_ADC_PollForConversion(&g_adc_handle, 10);    // 等待转换完成，超时 10ms
uint16_t adc_val = HAL_ADC_GetValue(&g_adc_handle); // 读取 ADC 结果
```
    - 注意：每次调用 `HAL_ADC_Start` 后，ADC 执行一次转换，转换结束后需重新调用才能再次转换。
12. **将原始值换算为电压并显示**
    - 计算公式：`电压 = adc_val * (3.3 / 4096)`
    - 12位分辨率：参考电压 3.3V，数字量范围 0~4095。
    - 显示时，用整数+3位小数的方式处理，更加直观。

---

### 四、实验结果

- 当 PA1 接 **GND** 时，LCD 显示 ADC 值约 **0**，电压约 **0.000V**。
- 当 PA1 接 **3.3V** 时，ADC 值约 **4095**，电压约 **3.300V**。
- 接中间电压（如通过电位器分压），ADC 值和电压呈线性关系，精度良好。

---

### 五、**重点、难点、要点总结**

### 🔴 重点

13. **ADC 初始化结构体各个成员的含义**
    - `DataAlign`、`ScanConvMode`、`ContinuousConvMode`、`ExternalTrigConv` 等必须结合单通道需求正确设置。
14. **ADC 时钟配置**
    - 不能超过 14MHz，一般采用 PCLK2 分频到 12MHz。
15. **HAL 库的 MSP 回调函数**
    - 在 `HAL_ADC_Init` 调用时会自动调用 `HAL_ADC_MspInit`，用于配置外设底层资源（时钟、引脚）。
16. **ADC 结果与电压的转换公式**
    - 必须牢记：`电压 = 读数值 ×（参考电压 / 4096）`

### 🟡 难点

17. **ADC 时钟树的理解**
    - 需要清楚 APB2 总线时钟经过分频器才送给 ADC，配置时容易忽略。
18. **采样时间的选取**
    - 采样时间影响精度和速度，要根据信号源内阻、需要的转换速率权衡。
19. **单次转换模式下的软件触发**
    - 必须每次转换前都调用 `HAL_ADC_Start`，初学者容易忘记导致读到的值不更新。
20. **浮点数在 LCD 上的拆分显示逻辑**
    - 将浮点电压拆成整数和小数两部分分别显示，涉及取整、乘法、浮点转整型等操作，是一个常见的综合应用技巧。

### 🟢 要点

- ADC 使用前必须**校准**，否则结果会有零点偏移。
- 引脚模式必须设为**模拟输入**，不能是普通 GPIO 模式。
- 如果使用多通道，则需使能扫描模式并配置规则组序列。
- `HAL_ADC_PollForConversion` 的第二个参数是超时时间（毫秒），要保证大于实际转换时间。
- LCD 显示时注意刷新位置，避免残留字符影响视觉效果。

---

### 六、**小扩展思考**

如果想改成**连续自动转换**怎么办？
只需修改初始化：

```c
g_adc_handle.Init.ContinuousConvMode = ENABLE;
```

然后 `HAL_ADC_Start` 一次，ADC 就会持续转换，在主循环直接用 `HAL_ADC_GetValue` 读最新值（不用反复 Start）。
但需要注意，连续模式下读取速度很快，要配合适当的延时或中断方式来平衡。

---

**总结：本次实验成功实现了 STM32F103ZET6 的 ADC1 通道1 单次转换电压采集，验证了 ADC 的配置流程和电压换算公式，为后续多通道、DMA 传输等高级应用打下坚实基础。**

你直接把这些整理到笔记里就行，不清楚的地方随时问老师！

[[adc.c]]

[[adc.h]]

[[main.c]]
