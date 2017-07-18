/*****************************************************************
已注释DMA中断；

******************************************************************/

#include "user_adc.h"
#include "platform.h"
#include "math.h"

volatile uint8_t u8ADF=0;

void ADC_Init(void)
{
	    // Enable channel 0
    ADC_Open(ADC, ADC_INPUT_MODE_SINGLE_END, ADC_OPERATION_MODE_SINGLE_CYCLE, 
		ADC_CH_5_MASK|ADC_CH_4_MASK|ADC_CH_3_MASK|ADC_CH_2_MASK|ADC_CH_1_MASK);//|ADC_CH_4_MASK|ADC_CH_3_MASK|ADC_CH_2_MASK|ADC_CH_1_MASK

    // Set reference voltage to AVDD
    ADC_SET_REF_VOLTAGE(ADC, ADC_REFSEL_POWER);

		ADC_POWER_ON(ADC);
    // Enable ADC ADC_IF interrupt
    ADC_EnableInt(ADC, ADC_ADF_INT);
    NVIC_EnableIRQ(ADC_IRQn);
}

void ADC_IRQHandler(void)
{
    uint32_t u32Flag;

    // Get ADC conversion finish interrupt flag
    u32Flag = ADC_GET_INT_FLAG(ADC, ADC_ADF_INT);

    if(u32Flag & ADC_ADF_INT)
        u8ADF = 1;

    ADC_CLR_INT_FLAG(ADC, u32Flag);
}

uint8_t ADC_convert (uint32_t* value)
{
		uint32_t u32Result[5]={0};
		float vol_temp[5]={0};
		// Power on ADC
		ON_ADC_POW;
		ON_D1_POW;
		ON_D2_POW;
		ON_D3_POW;
		ON_D4_POW;
		Delay(500);
		ADC_EnableInt(ADC, ADC_ADF_INT);
		ADC_START_CONV(ADC);
		while (u8ADF == 0);
		u8ADF = 0;
		for(uint8_t i=0;i<10;i++)
		{
			ADC_EnableInt(ADC, ADC_ADF_INT);
			ADC_START_CONV(ADC);
			while (u8ADF == 0);
			u8ADF = 0;
			u32Result[0] += ADC_GET_CONVERSION_DATA(ADC, 5);//电源电量检测
			
			u32Result[1] += ADC_GET_CONVERSION_DATA(ADC, 4);//测距传感器1检测
			u32Result[2] += ADC_GET_CONVERSION_DATA(ADC, 3);//测距传感器2检测
			u32Result[3] += ADC_GET_CONVERSION_DATA(ADC, 2);//测距传感器3检测
			u32Result[4] += ADC_GET_CONVERSION_DATA(ADC, 1);//测距传感器4检测
			ADC_STOP_CONV(ADC);
			//printf("Channel 5 conversion result is 0x%x\n",u32Result);
		}
		
		vol_temp[0] = u32Result[0]*3.3/40960;
		vol_temp[1] = u32Result[1]*3.3/40960;
		vol_temp[2] = u32Result[2]*3.3/40960;
		vol_temp[3] = u32Result[3]*3.3/40960;
		vol_temp[4] = u32Result[4]*3.3/40960;
		
		value[0] = (vol_temp[0]*2000);
		value[1] = (uint32_t)(59.828*pow(vol_temp[1],-1.684));//(uint32_t)(41.841*pow(vol_temp[2],-5.531));(2)
		value[2] = (uint32_t)(60.331*pow(vol_temp[2],-1.686));//(uint32_t)(33.455*pow(vol_temp[1],-4.209));(1)
		value[3] = (uint32_t)(51.773*pow(vol_temp[3],-1.526));//(uint32_t)(27.014*pow(vol_temp[4],-3.974));(4)
		value[4] = (uint32_t)(50.221*pow(vol_temp[4],-1.448));//(uint32_t)(29.569*pow(vol_temp[3],-3.144));(3)
		//printf("sensor2 value:[%d] ---\r\n",value[2]);
		OFF_ADC_POW;
		OFF_D1_POW;
		OFF_D2_POW;
		OFF_D3_POW;
		OFF_D4_POW;
		
//		for(uint8_t i=0;i<4;i++)
//		{
//			if(value[i+1] >24)
//			{
//				value[i+1]-=24;
//			}
//			else
//			{
//				value[i+1]=0;
//			}
//		}
		return 0;
}


uint8_t power_ADC_convert (uint32_t* value)
{
		uint32_t u32Result=0;
		float vol_temp = 0;
		// Power on ADC
		ON_ADC_POW;
		for(uint8_t i=0;i<10;i++)
		{
			ADC_EnableInt(ADC, ADC_ADF_INT);
			ADC_START_CONV(ADC);
			while (u8ADF == 0);
			u8ADF = 0;
			u32Result += ADC_GET_CONVERSION_DATA(ADC, 5);//电源电量检测
			ADC_DisableInt(ADC, ADC_ADF_INT);
			//printf("Channel 5 conversion result is 0x%x\n",u32Result);
		}
		vol_temp = u32Result*3.3/40960;
		value[0] = (vol_temp*2000);
		//printf("sensor2 value:[%d] ---\r\n",value[2]);
		OFF_ADC_POW;
		return 0;
}

