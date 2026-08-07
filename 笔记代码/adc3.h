#ifndef __ADC_H
#define __ADC_H

#include "./SYSTEM/sys/sys.h"
void adc_init(void);
uint32_t adc_get_result(void);
uint8_t lsens_get_val(void);
#endif
