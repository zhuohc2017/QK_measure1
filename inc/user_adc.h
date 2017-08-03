#ifndef __USER_ADC_H__
#define __USER_ADC_H__
#include "Nano100Series.h"

void ADC_Init(void);

uint8_t ADC_convert (uint32_t* value);

uint8_t power_ADC_convert (uint32_t* value);

uint8_t IRsensor_ADC_convert (uint16_t* value);
#endif


