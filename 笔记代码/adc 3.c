#include "./BSP/ADC/adc3.h"

ADC_HandleTypeDef g_adc_handle;

void adc_init(void)
{
    ADC_ChannelConfTypeDef adc_ch_conf;

    g_adc_handle.Instance = ADC3;
    g_adc_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    g_adc_handle.Init.ScanConvMode = ADC_SCAN_DISABLE;
    g_adc_handle.Init.ContinuousConvMode = ENABLE;
    g_adc_handle.Init.NbrOfConversion = 1;
    g_adc_handle.Init.DiscontinuousConvMode = DISABLE;
    g_adc_handle.Init.NbrOfDiscConversion = 0;
    g_adc_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    HAL_ADC_Init(&g_adc_handle);

    HAL_ADCEx_Calibration_Start(&g_adc_handle);

    adc_ch_conf.Channel = ADC_CHANNEL_6;
    adc_ch_conf.Rank = ADC_REGULAR_RANK_1;
    adc_ch_conf.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&g_adc_handle,&adc_ch_conf);
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC3)
    {
        GPIO_InitTypeDef gpio_init_struct;
        RCC_PeriphCLKInitTypeDef adc_clk_init = {0};

        __HAL_RCC_GPIOF_CLK_ENABLE();
        __HAL_RCC_ADC3_CLK_ENABLE();

        gpio_init_struct.Pin = GPIO_PIN_8;
        gpio_init_struct.Mode = GPIO_MODE_ANALOG;
        HAL_GPIO_Init(GPIOF,&gpio_init_struct);
        
        adc_clk_init.PeriphClockSelection = RCC_PERIPHCLK_ADC;
        adc_clk_init.AdcClockSelection = RCC_ADCPCLK2_DIV6;
        HAL_RCCEx_PeriphCLKConfig(&adc_clk_init);
    }
}

uint32_t adc_get_result(void)
{
    HAL_ADC_Start(&g_adc_handle);
    HAL_ADC_PollForConversion(&g_adc_handle,10);
    return (uint32_t)HAL_ADC_GetValue(&g_adc_handle);
}

uint8_t lsens_get_val(void)
{
    uint32_t temp_val;
    //temp得到adc读取的值
    temp_val = adc_get_result();
    //把4096个数字变成1~100;
    temp_val /= 40;
    //把96舍掉
    if(temp_val > 100) temp_val = 100;

    return (uint8_t)(100-temp_val);
}
