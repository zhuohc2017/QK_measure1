#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include "Nano100Series.h"
#include "user_usart.h"
#include "user_adc.h"
#include <string.h>
#include "w25q64.h"



#define BT_STATE_UP_Pin PA8

#define BT_RXD_Pin PB5

#define BT_TXD_Pin PB4

#define LS_EN_A_Pin PA13

#define LS_EN_B_Pin PA12

#define BT_RST_Pin PB6

#define BT_EN_Pin PB7

#define ADC_POW_PIN PA9

#define POW_DETC_Pin PA5


#define N_LS_IN1_Pin PA4

#define N_LS_IN2_Pin PA3

#define N_LS_IN3_PIN PA2

#define N_LS_IN4_PIN PA1



#define LS_PWEN1_Pin PC11

#define LS_PWEN2_Pin PC10

#define LS_PWEN3_Pin PC9

#define LS_PWEN4_Pin PC8


#define LS_SEL3_Pin PA6

#define LS_SEL2_Pin PC7

#define LS_SEL1_Pin PC6

#define LS_SEL0_Pin PC15

#define BT_STATE_DOWN_Pin PB14


#define SPK_Pin PD6


#define LED4_Pin PD7

#define LED3_Pin PD14



#define N_LS_CTR1_Pin PE5

#define N_LS_CTR2_Pin PB11

#define N_LS_CTR3_Pin PB10

#define N_LS_CTR4_Pin PB9

#define INDICATE_POWER_Pin PC14

#define ON_ADC_POW			ADC_POW_PIN = 1
#define OFF_ADC_POW			ADC_POW_PIN = 0

#define ON_D1_POW				N_LS_CTR1_Pin = 1 //低电平有效

#define OFF_D1_POW			N_LS_CTR1_Pin = 0

#define ON_D2_POW				N_LS_CTR2_Pin = 1

#define OFF_D2_POW			N_LS_CTR2_Pin = 0

#define ON_D3_POW				N_LS_CTR3_Pin = 1

#define OFF_D3_POW			N_LS_CTR3_Pin = 0

#define ON_D4_POW				N_LS_CTR4_Pin = 1

#define OFF_D4_POW			N_LS_CTR4_Pin = 0

#define ON_INDI_POW  		INDICATE_POWER_Pin = 1
#define OFF_INDI_POW 		INDICATE_POWER_Pin = 0

#define ON_LED_BLE  				LED4_Pin = 1//battery LED
#define OFF_LED_BLE 				LED4_Pin = 0
#define F_N_LED_BLE 				LED4_Pin = ~LED4_Pin

#define ON_LED_BAT  				LED3_Pin = 1
#define OFF_LED_BAT 				LED3_Pin = 0
#define F_N_LED_BAT 				LED3_Pin = ~LED3_Pin


#define ON_SPK  				SPK_Pin = 1
#define OFF_SPK 				SPK_Pin = 0

#define ON_LS_EN_A_ON   LS_EN_A_Pin = 0
#define ON_LS_EN_A_OFF  LS_EN_A_Pin = 1


#define ON_LS_EN_B_ON   LS_EN_B_Pin = 0
#define ON_LS_EN_B_OFF  LS_EN_B_Pin = 1
/******************DIANyuan*****************/

#define ON1_PWEN  		  LS_PWEN1_Pin = 1
#define OFF1_PWEN 		  LS_PWEN1_Pin = 0

#define ON2_PWEN  			LS_PWEN2_Pin = 1
#define OFF2_PWEN 			LS_PWEN2_Pin = 0

#define ON3_PWEN  			LS_PWEN3_Pin = 1
#define OFF3_PWEN 			LS_PWEN3_Pin = 0

#define ON4_PWEN  			LS_PWEN4_Pin = 1
#define OFF4_PWEN 			LS_PWEN4_Pin = 0

void SYS_Init(void);

void Timer1_Init(void);

void Delay(uint32_t ms);

void GPIO_Init(void);
void GPIO_Int_Enable(void);
void GPIO_Int_Disable(void); 

void error_handle(void);
void start_handle(void);
void end_handle(void);

uint8_t power_on(uint8_t num);
uint8_t power_off(uint8_t num);
uint16_t usart_send_1_4_ctrl(uint8_t code);
uint16_t usart_receive_1_16_ctrl(uint8_t code);

void select_usart(uint8_t a,uint8_t b,uint8_t c,uint8_t d);

uint8_t set_ID(uint8_t* ID);
uint8_t set_err(uint8_t *err);
uint8_t set_IR(uint8_t *IR);
void set_bt_name(void);

uint8_t ADC_convert (uint32_t* value);


void Leave_PowerDown(void);

void enter_stop(void);

#endif

