#ifndef __USER_USART_H__
#define __USER_USART_H__

#include "Nano100Series.h"
#define SIZE_LEN 25

typedef struct 
{
	volatile uint8_t  receivebuf[SIZE_LEN];
	volatile uint8_t  receive_cnt;
	uint8_t  flag_complete;
	uint16_t tim_cnt;
	uint8_t  flag_update;
	uint8_t  flag_lock;
	void (*reset)(void);
}USART_struct;

void Timer0_Init(void);

void UART0_Init(void);

void UART1_Init(void);

void SENSOR_usart_service(void);

void BLE_usart_service(void);

#endif



