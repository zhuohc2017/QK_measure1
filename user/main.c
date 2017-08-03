/******************************************************************************
 * @file     main.c
 * @version  V1.00
 * $Revision: 3 $
 * $Date: 14/09/11 7:08p $
 * @brief    A project template for Nano100BN series MCUs
 *
 * @note
 * Copyright (C) 2014 Nuvoton Technology Corp. All rights reserved.
 2017-07-17
*****************************************************************************/

//#include "platform.h"
#include "app.h"

#include "time.h"

//#define DEBUG

uint16_t wakeup_timeout = 600;//超时时间设置，120*0.5=60
uint16_t wakeup_cnt = 0;
uint8_t flag_wakeup = 1;
uint8_t flag_con_discon = 0;
uint8_t flag_bt_timeout = 0;
uint32_t p_battery[5]={0};
/* USER CODE BEGIN PV */
extern uint8_t flag_BLE_OK;//蓝牙检测标志
extern uint8_t flag_BLE_newline;
extern uint8_t flag_BLE_CMD;
extern uint8_t  Self_test_flag;
extern USART_struct BLE_usart;
extern USART_struct SENSOR_usart;
extern uint16_t flag_detect;
extern uint8_t u8ADF;
extern uint16_t flag_poweron;
extern uint8_t IR_co[8];

volatile int32_t   g_bAlarm  = FALSE;
int main()
{
		uint16_t temp;
		
		S_RTC_TIME_DATA_T sCurTime;
		
		SYS_Init();
	
		GPIO_Init();
	
		MX_SPI1_Init();
	
		ADC_Init();
	
		Timer0_Init();
		Timer1_Init();
    /* Init UART to 115200-8n1 for print message */
		UART1_Init();
		UART0_Init();
		SYS_UnlockReg();
	  /* Time Setting */
    sCurTime.u32Year       = 2017;
    sCurTime.u32Month      = 2;
    sCurTime.u32Day        = 7;
    sCurTime.u32Hour       = 0;
    sCurTime.u32Minute     = 0;
    sCurTime.u32Second     = 0;
    sCurTime.u32DayOfWeek  = RTC_TUESDAY;
    sCurTime.u32TimeScale  = RTC_CLOCK_24;
   
		temp = SPI_Flash_ReadID();
#ifdef DEBUG
		printf("Hello World\nSPIflash ID: %x \r\n",temp);
#endif
		SPI_Flash_ReadBuffer(IR_co,0x7FD000,8);
		ADC_convert(p_battery);
		Delay(1000);
		
		RTC_Open(&sCurTime);
		
		RTC_SetTickPeriod(RTC_TICK_1_2_SEC);
    /* Enable RTC Alarm Interrupt */
		RTC_EnableInt(RTC_RIER_TIER_Msk);
//    RTC_EnableInt(RTC_RIER_AIER_Msk);
		NVIC_EnableIRQ(RTC_IRQn);
		RTC_ENABLE_TICK_WAKEUP();
//		power_on(0);
//		power_on(1);
//		power_on(2);
//		power_on(3);
	//自检
	//BT_RST_Pin = 0;//使蓝牙不可以连接
	//set_bt_name();
	//Self_test();
		
	BLE_usart.reset();
	
	while(flag_poweron <= 500);
	BLE_usart.reset();
	GPIO_Int_Enable();
	//BT_RST_Pin = 1;//使蓝牙可以连接
  NVIC_DisableIRQ(TMR1_IRQn);
	TIMER_DisableInt(TIMER1);
// WDT timeout every 2^14 WDT clock, disable system reset, disable wake up system
	WDT_Open(WDT_TIMEOUT_2POW16, WDT_RESET_DELAY_1026CLK, TRUE, FALSE);
	
	//12   407ms
	//14   1.638
	//16   6.553
	//18   26
	while (1)
  {
#ifdef DEBUG
		printf("while ---\r\n");
#endif
		
		if(flag_con_discon == 1)
		{
			BLE_usart.reset();
			SENSOR_usart.reset();
			flag_con_discon=0;
			end_handle();
		}
		if(flag_wakeup == 1)
		{
#ifdef DEBUG
		printf("wake-%d!---\r\n",wakeup_cnt);
#endif			
			if(BLE_usart.flag_complete==1)
			{
				if(flag_BLE_CMD == 1)
				{
					Execute_Cmd((uint8_t*)BLE_usart.receivebuf);
					flag_BLE_CMD = 0;
					BLE_usart.reset();
					SENSOR_usart.reset();
					wakeup_cnt = 0;
				}
				else if(flag_BLE_newline == 1)
				{
					flag_BLE_newline = 0;
					BLE_usart.reset();
					SENSOR_usart.reset();
				}
				else
				{
					error_handle();
					BLE_usart.reset();
					SENSOR_usart.reset();
				}
			}
			
			Delay(500);
			wakeup_cnt++;
			if(wakeup_cnt >wakeup_timeout)//
			{
				flag_wakeup = 0;
				flag_bt_timeout=1;
			}
		}
		else
		{
			wakeup_cnt=0;
			//flag_detect = 0;
			if(flag_bt_timeout==1)
			{
				flag_bt_timeout=0;
				BT_RST_Pin = 0;//蓝牙复位
				Delay(100);
				BT_RST_Pin = 1;//蓝牙复位
			}
#ifdef DEBUG
		printf("sleep ---\r\n");
#endif
			Delay(500);
			//enter_stop();
		}

		// Reset WDT and clear time out flag
		//WDT_CLEAR_TIMEOUT_INT_FLAG();
		
		WDT_RESET_COUNTER();
		
		power_ADC_convert(p_battery);
		
		if( p_battery[0]<3700)
		{
			F_N_LED_BAT;
		}
		else if(p_battery[0]>3700)
		{
			OFF_LED_BAT;
		}
		if(Self_test_flag&8)
		{
			F_N_LED_BLE;
		}
  }
}

/*** (C) COPYRIGHT 2014 Nuvoton Technology Corp. ***/
