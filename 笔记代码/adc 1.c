#include "./BSP/ADC/adc.h"

ADC_HandleTypeDef g_adc_handle;

void adc_temperature_init(void)
{
    ADC_ChannelConfTypeDef adc_ch_conf;
    g_adc_handle.Instance = ADC1;
    g_adc_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    g_adc_handle.Init.ScanConvMode = ADC_SCAN_DISABLE;
    g_adc_handle.Init.ContinuousConvMode = ENABLE;
    g_adc_handle.Init.NbrOfConversion = 1;
    g_adc_handle.Init.DiscontinuousConvMode = DISABLE;
    g_adc_handle.Init.NbrOfDiscConversion = 0;
    g_adc_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    HAL_ADC_Init(&g_adc_handle);

    HAL_ADCEx_Calibration_Start(&g_adc_handle);

    adc_ch_conf.Channel = ADC_CHANNEL_16;
    adc_ch_conf.Rank = ADC_REGULAR_RANK_1;
    adc_ch_conf.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&g_adc_handle,&adc_ch_conf);
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    if(hadc->Instance == ADC1)
    {
        RCC_PeriphCLKInitTypeDef adc_clk_init = {0};

        __HAL_RCC_ADC1_CLK_ENABLE();

        adc_clk_init.AdcClockSelection = RCC_ADCPCLK2_DIV6;
        adc_clk_init.PeriphClockSelection = RCC_PERIPHCLK_ADC;
        HAL_RCCEx_PeriphCLKConfig(&adc_clk_init);
    }

}

uint32_t adc_get_result(void)
{
    HAL_ADC_Start(&g_adc_handle);
    HAL_ADC_PollForConversion(&g_adc_handle,10);
    return (uint32_t)HAL_ADC_GetValue(&g_adc_handle);
}

short adc_get_temperature(void)
{
    uint32_t adcx;
    short result;
    double temperature;

    adcx = adc_get_result();
    temperature = adcx * (3.3 / 4096);
    //1.43是V25常量值 0.0043 是 4.3mv转为V得来的
    temperature = (1.43 - temperature) /0.0043 +25;
    result = temperature *= 100;
    return result;

}
