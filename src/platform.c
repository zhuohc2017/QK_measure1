#include "platform.h"
#include "Nano100Series.h"
#include <stdio.h>

//#define DEBUG

uint16_t flag_detect = 0;
uint16_t flag_poweron = 0;
uint8_t flag_500ms_tick = 0;
uint8_t temp[38]={0};
uint8_t ID_temp[38]={0};
extern USART_struct SENSOR_usart;
extern USART_struct BLE_usart;
extern uint32_t p_battery[5];
extern uint8_t  Self_test_flag;
extern uint8_t  flag_wakeup;
extern uint8_t  flag_con_discon;

extern uint8_t ERR_Com[32];
extern uint8_t IR_co[8];
/* Global variables */
__IO int32_t   _Wakeup_Flag = 0;    /* 1 indicates system wake up from power down mode */
__IO uint32_t  _Pin_Setting[11];    /* store Px_H_MFP and Px_L_MFP */
__IO uint32_t  _PullUp_Setting[6];  /* store GPIOx_PUEN */


/**
  * @brief  PDWU IRQHandler.
  * @param  None.
  * @return None.
  */
void PDWU_IRQHandler()
{   
    CLK->WK_INTSTS = 1; /* clear interrupt status */
    Leave_PowerDown();
}


/**
  * @brief  RTC IRQHandler.
  * @param  None.
  * @return None.
  */

void RTC_IRQHandler()
{
    /* RTC Tick interrupt */
    if ((RTC->RIER & RTC_RIER_TIER_Msk) && (RTC->RIIR & RTC_RIIR_TIF_Msk))
		{
#ifdef DEBUG
			printf("RTC_INQ---\r\n");
#endif
			
        RTC->RIIR = RTC_RIIR_TIF_Msk;
				RTC_CLEAR_TICK_INT_FLAG();
//				RTC_DisableInt(RTC_RIER_TIER_Msk);
//				RTC_DISABLE_TICK_WAKEUP();
//				NVIC_DisableIRQ(RTC_IRQn);
    }

    /* RTC Alarm interrupt */
    if ((RTC->RIER & RTC_RIER_AIER_Msk) && (RTC->RIIR & RTC_RIIR_AIF_Msk)) 
		{
        RTC->RIIR = RTC_RIIR_AIF_Msk;       
    }

    if ((RTC->RIER & RTC_RIER_SNOOPIER_Msk) && (RTC->RIIR & RTC_RIIR_SNOOPIF_Msk)) 
		{ /* snooper interrupt occurred */
        RTC->RIIR = RTC_RIIR_SNOOPIF_Msk;
    }
}

/**
  * @brief  Store original setting of multi-function pin selection.
  * @param  None.
  * @return None.
  */
void SavePinSetting()
{
    /* Save Pin selection setting */
    _Pin_Setting[0] = SYS->PA_L_MFP;
    _Pin_Setting[1] = SYS->PA_H_MFP;
    _Pin_Setting[2] = SYS->PB_L_MFP;
    _Pin_Setting[3] = SYS->PB_H_MFP;
    _Pin_Setting[4] = SYS->PC_L_MFP;
    _Pin_Setting[5] = SYS->PC_H_MFP;
    _Pin_Setting[6] = SYS->PD_L_MFP;
    _Pin_Setting[7] = SYS->PD_H_MFP;
    _Pin_Setting[8] = SYS->PE_L_MFP;
    _Pin_Setting[9] = SYS->PE_H_MFP;
    _Pin_Setting[10] = SYS->PF_L_MFP;

    /* Save Pull-up setting */
    _PullUp_Setting[0] =  PA->PUEN;
    _PullUp_Setting[1] =  PB->PUEN;
    _PullUp_Setting[2] =  PC->PUEN;
    _PullUp_Setting[3] =  PD->PUEN;
    _PullUp_Setting[4] =  PE->PUEN;
    _PullUp_Setting[5] =  PF->PUEN;
}

/**
  * @brief  Restore original setting of multi-function pin selection.
  * @param  None.
  * @return None.
  */
void RestorePinSetting()
{
    /* Restore Pin selection setting */
    SYS->PA_L_MFP = _Pin_Setting[0];
    SYS->PA_H_MFP = _Pin_Setting[1];
    SYS->PB_L_MFP = _Pin_Setting[2];
    SYS->PB_H_MFP = _Pin_Setting[3];
    SYS->PC_L_MFP = _Pin_Setting[4];
    SYS->PC_H_MFP = _Pin_Setting[5];
    SYS->PD_L_MFP = _Pin_Setting[6];
    SYS->PD_H_MFP = _Pin_Setting[7];
    SYS->PE_L_MFP = _Pin_Setting[8];
    SYS->PE_H_MFP = _Pin_Setting[9];
    SYS->PF_L_MFP = _Pin_Setting[10];

    /* Restore Pull-up setting */
    PA->PUEN = _PullUp_Setting[0];
    PB->PUEN = _PullUp_Setting[1];
    PC->PUEN = _PullUp_Setting[2];
    PD->PUEN = _PullUp_Setting[3];
    PE->PUEN = _PullUp_Setting[4];
    PF->PUEN = _PullUp_Setting[5];
}


void Leave_PowerDown()
{
		//printf(">>>>Leave Power Down---\r\n");
    /* Restore pin setting */
    RestorePinSetting();

    /* Set PF.0 and PF.1 to ICE Data and Clock */
    SYS->PF_L_MFP |= 0x00000077;
		
    /* Enable LCD clock */
    //CLK->APBCLK |= CLK_APBCLK_LCD_EN;
		//SYS_Init();
	
		//printf("<<<<Leave Power Down---\r\n");
//			GPIO_Init();
//			ADC_Init();
}
void Delay(uint32_t ms)
{
	CLK_SysTickDelay(1000*ms);
}

uint8_t set_ID(uint8_t *ID)
{	
	SPI_Flash_ReadBuffer(ID_temp,0x7FE000,38);
	Delay(100);
	W25QXX_Erase_Sector(0x7FE);
	Delay(100);
	SPI_Flash_Writebuffer(&ID_temp[6],0x7FE006,32);
	Delay(100);
	SPI_Flash_Writebuffer(ID+2,0x7FE000,6);
	Delay(100);
	SPI_Flash_Writebuffer(ID,0x7FE026,2);
	Delay(100);
	return 0;
}

uint8_t set_err(uint8_t *err)
{	
	SPI_Flash_ReadBuffer(temp,0x7FE000,38);
	W25QXX_Erase_Sector(0x7FE);  
	Delay(1000);
	//
	//首先判断原来存储的误差值，跟新设定的误差符号是否相同，如果相同
	//如果是第一次设置，应注意符号
	if(temp[6+2*(err[0]-1)]!='-'&&temp[6+2*(err[0]-1)]!='+')
	{
		temp[6+2*(err[0]-1)]=err[1];
		temp[7+2*(err[0]-1)]=err[2];
	}
	else
	{
		if(temp[6+2*(err[0]-1)]!=err[1])//如果不相等，符号跟数值大的相同
		{
			if(temp[7+2*(err[0]-1)]<err[2])
			{
				temp[6+2*(err[0]-1)]=err[1];
				temp[7+2*(err[0]-1)]=err[2]-temp[7+2*(err[0]-1)];				
			}
			else
			{
				//temp[6+2*(err[0]-1)]不变
				temp[7+2*(err[0]-1)]-=err[2];
			}
		}
		else//如果符号相等
		{
			temp[6+2*(err[0]-1)]=err[1];
			temp[7+2*(err[0]-1)]+=err[2];
		}
	}
	
	if(temp[0]==0xFF&&temp[1]==0xFF&&temp[2]==0xFF
		&&temp[3]==0xFF&&temp[4]==0xFF&&temp[5]==0xFF)
	{
		SPI_Flash_Writebuffer(&temp[6],0x7FE006,32);
		Delay(100);
	}
	else
	{
		SPI_Flash_Writebuffer(temp,0x7FE000,38);
		Delay(100);
	}
	SPI_Flash_ReadBuffer(ERR_Com,0x7FE006,32);
	Delay(100);
	return 0;
}


uint8_t set_IR(uint8_t *IR)
{
	uint8_t ir_temp[8]={0};
	SPI_Flash_ReadBuffer(ir_temp,0x7FD000,8);
	Delay(100);
	W25QXX_Erase_Sector(0x7FD);  
	Delay(1000);
	ir_temp[2*(IR[0]-1)] = IR[1];
	ir_temp[2*(IR[0]-1)+1] = IR[2];
	SPI_Flash_Writebuffer(ir_temp,0x7FD000,8);
	Delay(100);
	SPI_Flash_ReadBuffer(IR_co,0x7FD000,8);
	Delay(100);
}
void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable External XTAL (4~24 MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HXT_EN_Msk);
	
		CLK_EnableXtalRC(CLK_PWRCTL_LXT_EN_Msk);
    /* Waiting for 12MHz clock ready */
    CLK_WaitClockReady( CLK_CLKSTATUS_HXT_STB_Msk);
		
		CLK_WaitClockReady( CLK_CLKSTATUS_LXT_STB_Msk);

    /* Switch HCLK clock source to HXT */
    CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HXT,CLK_HCLK_CLK_DIVIDER(1));

    /* Enable IP clock */
    CLK_EnableModuleClock(UART0_MODULE);
    
		CLK_EnableModuleClock(WDT_MODULE);
		
		CLK_EnableModuleClock(UART1_MODULE);
	
	  CLK_EnableModuleClock(SPI1_MODULE);
		
		CLK_EnableModuleClock(ADC_MODULE);
		
    CLK_EnableModuleClock(TMR0_MODULE);
		CLK_EnableModuleClock(TMR1_MODULE);
		
		CLK_EnableModuleClock(RTC_MODULE);

    /* Select IP clock source */
    CLK_SetModuleClock(WDT_MODULE, 0, 0);
		CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART_S_HXT, CLK_UART_CLK_DIVIDER(1));
		CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART_S_HXT, CLK_UART_CLK_DIVIDER(1));
		
		CLK_SetModuleClock(SPI1_MODULE, CLK_CLKSEL2_SPI1_S_HCLK, 0);
	
    CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0_S_HXT, 0);
		CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1_S_HXT, 0);
    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set GPB multi-function pins for UART1 RXD and TXD  for---------------------------------------------------------BT*/
    SYS->PB_L_MFP &= ~(SYS_PB_L_MFP_PB4_MFP_Msk | SYS_PB_L_MFP_PB5_MFP_Msk);
    SYS->PB_L_MFP |= (SYS_PB_L_MFP_PB4_MFP_UART1_RX | SYS_PB_L_MFP_PB5_MFP_UART1_TX);
		
		/*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PA multi-function pins for UART0 RXD and TXD  */
    SYS->PA_H_MFP &= ~(SYS_PA_H_MFP_PA14_MFP_Msk | SYS_PA_H_MFP_PA15_MFP_Msk);
		SYS->PA_H_MFP |= (SYS_PA_H_MFP_PA14_MFP_UART0_RX | SYS_PA_H_MFP_PA15_MFP_UART0_TX);
		
		/* Set multi function pin for SPI0 */
		
    SYS->PB_L_MFP &= ~(SYS_PB_L_MFP_PB0_MFP_Msk | SYS_PB_L_MFP_PB1_MFP_Msk | SYS_PB_L_MFP_PB2_MFP_Msk 
										| SYS_PB_L_MFP_PB3_MFP_Msk
                     );
										 
    SYS->PB_L_MFP |= (SYS_PB_L_MFP_PB3_MFP_SPI1_SS0 | SYS_PB_L_MFP_PB2_MFP_SPI1_SCLK | SYS_PB_L_MFP_PB1_MFP_SPI1_MISO0 
										| SYS_PB_L_MFP_PB0_MFP_SPI1_MOSI0
                     );
		
		
		/* Set PA multi-function pins for ADC */
    SYS->PA_L_MFP &= ~(SYS_PA_L_MFP_PA1_MFP_Msk | SYS_PA_L_MFP_PA2_MFP_Msk | SYS_PA_L_MFP_PA3_MFP_Msk | SYS_PA_L_MFP_PA4_MFP_Msk | SYS_PA_L_MFP_PA5_MFP_Msk);
    SYS->PA_L_MFP |= SYS_PA_L_MFP_PA1_MFP_ADC_CH1 | SYS_PA_L_MFP_PA2_MFP_ADC_CH2 | SYS_PA_L_MFP_PA3_MFP_ADC_CH3 | SYS_PA_L_MFP_PA4_MFP_ADC_CH4| SYS_PA_L_MFP_PA5_MFP_ADC_CH5;

    /* Disable PA.0 PA.1 PA.2 digital input path */
    PA->OFFD |= (((1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)) << GP_OFFD_OFFD_Pos);
		
		
    /* Lock protected registers */
    SYS_LockReg();
}


void GPIO_Int_Enable(void)
{
	GPIO_EnableInt(PA, 8, GPIO_INT_RISING);
	NVIC_EnableIRQ(GPABC_IRQn);
	GPIO_EnableInt(PB, 14, GPIO_INT_RISING);
	NVIC_EnableIRQ(GPABC_IRQn);		
}
void GPIO_Int_Disable(void)
{
	GPIO_DisableInt(PA, 8);
	NVIC_DisableIRQ(GPABC_IRQn);
	GPIO_DisableInt(PB, 14);
	NVIC_DisableIRQ(GPABC_IRQn);		
}

void GPIO_Init()
{
		GPIO_SetMode(PA, BIT8, GPIO_PMD_INPUT);//			#define BT_STATE_UP_Pin PA8   中断输入
		GPIO_DISABLE_PULL_UP(PA, BIT8);
		

//		GPIO_SetMode(PB, BIT5, GPIO_PMD_OUTPUT);//			#define BT_RXD_Pin PB5
//		GPIO_SetMode(PB, BIT4, GPIO_PMD_OUTPUT);//			#define BT_TXD_Pin PB4
		GPIO_SetMode(PA, BIT13, GPIO_PMD_OUTPUT);//			#define LS_EN_A_Pin PA13
		GPIO_SetMode(PA, BIT12, GPIO_PMD_OUTPUT);//			#define LS_EN_B_Pin PA12
	
		GPIO_SetMode(PB, BIT6, GPIO_PMD_OUTPUT);//			#define BT_RST_Pin PB6
		GPIO_SetMode(PB, BIT7, GPIO_PMD_OUTPUT);//			#define BT_EN_Pin PB7
	
		GPIO_SetMode(PA, BIT9, GPIO_PMD_OUTPUT);//			#define ADC_POW_PIN PA9

//		GPIO_SetMode(PA, BIT0, GPIO_PMD_OUTPUT);//			#define POW_DETC_Pin PA5
//		GPIO_SetMode(PA, BIT0, GPIO_PMD_OUTPUT);//			#define N_LS_IN1_Pin PA4
//		GPIO_SetMode(PA, BIT0, GPIO_PMD_OUTPUT);//			#define N_LS_IN2_Pin PA3
//		GPIO_SetMode(PA, BIT0, GPIO_PMD_OUTPUT);//			#define N_LS_IN3_PIN PA2
//		GPIO_SetMode(PA, BIT0, GPIO_PMD_OUTPUT);//			#define N_LS_IN4_PIN PA1
		
		GPIO_SetMode(PC, BIT11, GPIO_PMD_OUTPUT);//			#define LS_PWEN1_Pin PC11
		GPIO_SetMode(PC, BIT10, GPIO_PMD_OUTPUT);//			#define LS_PWEN2_Pin PC10
		GPIO_SetMode(PC, BIT9, GPIO_PMD_OUTPUT);//			#define LS_PWEN3_Pin PC9
		GPIO_SetMode(PC, BIT8, GPIO_PMD_OUTPUT);//			#define LS_PWEN4_Pin PC8
		
		GPIO_SetMode(PA, BIT6, GPIO_PMD_OUTPUT);//			#define LS_SEL3_Pin PA6
		GPIO_SetMode(PC, BIT7, GPIO_PMD_OUTPUT);//			#define LS_SEL2_Pin PC7
		GPIO_SetMode(PC, BIT6, GPIO_PMD_OUTPUT);//			#define LS_SEL1_Pin PC6
		GPIO_SetMode(PC, BIT15, GPIO_PMD_OUTPUT);//			#define LS_SEL0_Pin PC15
		
		
		GPIO_SetMode(PB, BIT14, GPIO_PMD_INPUT); //			#define BT_STATE_DOWN_Pin PB14		
		GPIO_DISABLE_PULL_UP(PB, BIT14);

		
		GPIO_SetMode(PD, BIT6, GPIO_PMD_OUTPUT);//			#define SPK_Pin PD6
		GPIO_SetMode(PC, BIT14, GPIO_PMD_OUTPUT);//			#define LASER_CTR_Pin PC14
		GPIO_SetMode(PD, BIT7, GPIO_PMD_OUTPUT);//			#define LED4_Pin PD7
		GPIO_SetMode(PD, BIT14, GPIO_PMD_OUTPUT);//			#define LED3_Pin PD14
		GPIO_SetMode(PE, BIT5, GPIO_PMD_OUTPUT);//			#define N_LS_CTR1_Pin PE5
		GPIO_SetMode(PB, BIT11, GPIO_PMD_OUTPUT);//			#define N_LS_CTR2_Pin GPIO_PIN_13
		GPIO_SetMode(PB, BIT10, GPIO_PMD_OUTPUT);//			#define N_LS_CTR3_Pin GPIO_PIN_14
		GPIO_SetMode(PB, BIT9, GPIO_PMD_OUTPUT);//			#define N_LS_CTR4_Pin GPIO_PIN_15
		
		OFF_ADC_POW;
		OFF_INDI_POW;
		
		OFF1_PWEN;
		OFF2_PWEN;
		OFF3_PWEN;
		OFF4_PWEN;
		
		ON_D1_POW;
		ON_D2_POW;
		ON_D3_POW;
		ON_D4_POW;
		
		LS_SEL0_Pin = 0;
		
		LS_SEL1_Pin = 0;
		
		LS_SEL2_Pin = 0;
		
		LS_SEL3_Pin = 0;
		

		BT_EN_Pin = 1;

		GPIO_Int_Disable();
		ON_LS_EN_A_OFF;
		ON_LS_EN_B_OFF;
		
		OFF_SPK;
		OFF_LED_BAT;
		OFF_LED_BLE;
}

void GPIO_sleep_config()
{
//		GPIO_SetMode(PA, BIT8, GPIO_PMD_INPUT);//			#define BT_STATE_UP_Pin PA8   中断输入
//		GPIO_DISABLE_PULL_UP(PA, BIT8);
		

		GPIO_SetMode(PB, BIT5, GPIO_PMD_INPUT);//			#define BT_RXD_Pin PB5
		GPIO_SetMode(PB, BIT4, GPIO_PMD_INPUT);//			#define BT_TXD_Pin PB4
		GPIO_SetMode(PB, BIT6, GPIO_PMD_INPUT);//			#define BT_RST_Pin PB6
		GPIO_SetMode(PB, BIT7, GPIO_PMD_INPUT);//			#define BT_EN_Pin PB7
	
		GPIO_ENABLE_PULL_UP(PB, BIT4|BIT5|BIT6|BIT7);
	
		GPIO_SetMode(PA, BIT13, GPIO_PMD_INPUT);//			#define LS_EN_A_Pin PA13
		GPIO_SetMode(PA, BIT12, GPIO_PMD_INPUT);//			#define LS_EN_B_Pin PA12
		GPIO_ENABLE_PULL_UP(PA, BIT12|BIT13);
	
		GPIO_SetMode(PA, BIT9, GPIO_PMD_INPUT);//			#define ADC_POW_PIN PA9
		GPIO_DISABLE_PULL_UP(PA, BIT9);
//		GPIO_SetMode(PA, BIT0, GPIO_PMD_OUTPUT);//			#define POW_DETC_Pin PA5
//		GPIO_SetMode(PA, BIT0, GPIO_PMD_OUTPUT);//			#define N_LS_IN1_Pin PA4
//		GPIO_SetMode(PA, BIT0, GPIO_PMD_OUTPUT);//			#define N_LS_IN2_Pin PA3
//		GPIO_SetMode(PA, BIT0, GPIO_PMD_OUTPUT);//			#define N_LS_IN3_PIN PA2
//		GPIO_SetMode(PA, BIT0, GPIO_PMD_OUTPUT);//			#define N_LS_IN4_PIN PA1
		
		GPIO_SetMode(PC, BIT11, GPIO_PMD_INPUT);//			#define LS_PWEN1_Pin PC11
		GPIO_SetMode(PC, BIT10, GPIO_PMD_INPUT);//			#define LS_PWEN2_Pin PC10
		GPIO_SetMode(PC, BIT9, GPIO_PMD_INPUT);//			#define LS_PWEN3_Pin PC9
		GPIO_SetMode(PC, BIT8, GPIO_PMD_INPUT);//			#define LS_PWEN4_Pin PC8
		GPIO_DISABLE_PULL_UP(PC, BIT8|BIT9|BIT10|BIT11);
		
		
		GPIO_SetMode(PA, BIT6, GPIO_PMD_INPUT);//			#define LS_SEL3_Pin PA6
		GPIO_SetMode(PC, BIT7, GPIO_PMD_INPUT);//			#define LS_SEL2_Pin PC7
		GPIO_SetMode(PC, BIT6, GPIO_PMD_INPUT);//			#define LS_SEL1_Pin PC6
		GPIO_SetMode(PC, BIT15, GPIO_PMD_INPUT);//			#define LS_SEL0_Pin PC15
		GPIO_ENABLE_PULL_UP(PC,BIT6|BIT7|BIT15);
		GPIO_ENABLE_PULL_UP(PA,BIT6);
		
//		GPIO_SetMode(PB, BIT14, GPIO_PMD_INPUT); //			#define BT_STATE_DOWN_Pin PB14		
//		GPIO_DISABLE_PULL_UP(PB, BIT14);
//		GPIO_EnableInt(PB, 14, GPIO_INT_RISING);
//		NVIC_EnableIRQ(GPABC_IRQn);		
		
		GPIO_SetMode(PD, BIT6, GPIO_PMD_INPUT);//			#define SPK_Pin PD6
		GPIO_DISABLE_PULL_UP(PD, BIT6);
		
		GPIO_SetMode(PC, BIT14, GPIO_PMD_INPUT);//			#define LASER_CTR_Pin PC14
		GPIO_ENABLE_PULL_UP(PC,BIT14);
		
		
		GPIO_SetMode(PD, BIT7, GPIO_PMD_INPUT);//			#define LED4_Pin PD7
		GPIO_DISABLE_PULL_UP(PD, BIT7);
		
		
		GPIO_SetMode(PD, BIT14, GPIO_PMD_INPUT);//			#define LED3_Pin PD14
		GPIO_DISABLE_PULL_UP(PD, BIT14);
		
		
		GPIO_SetMode(PE, BIT5, GPIO_PMD_INPUT);//			#define N_LS_CTR1_Pin PE5
		
		GPIO_SetMode(PB, BIT11, GPIO_PMD_INPUT);//			#define N_LS_CTR2_Pin GPIO_PIN_13
		
		GPIO_SetMode(PB, BIT10, GPIO_PMD_INPUT);//			#define N_LS_CTR3_Pin GPIO_PIN_14
		
		GPIO_SetMode(PB, BIT9, GPIO_PMD_INPUT);//			#define N_LS_CTR4_Pin GPIO_PIN_15
		GPIO_ENABLE_PULL_UP(PB,BIT9|BIT10|BIT11);
		GPIO_ENABLE_PULL_UP(PE,BIT5);
}



void enter_stop()
{
#ifdef DEBUG
		printf("enter_stop ---\r\n");
#endif

//		RTC_EnableInt(RTC_RIER_TIER_Msk);
//		NVIC_EnableIRQ(RTC_IRQn);
//		RTC_ENABLE_TICK_WAKEUP();
	/* Back up original setting */
    SavePinSetting();

    /* Set function pin to GPIO mode */
    SYS->PA_L_MFP &= 0;
    SYS->PA_H_MFP &= 0x0000000F;//中断引脚PA8
    SYS->PB_L_MFP &= 0;
    SYS->PB_H_MFP &= 0x0F000000;//中断引脚PB14
    SYS->PC_L_MFP &= 0;
    SYS->PC_H_MFP &= 0;
    SYS->PD_L_MFP &= 0;
    SYS->PD_H_MFP &= 0;
    SYS->PE_L_MFP &= 0;
    SYS->PE_H_MFP &= 0;
    SYS->PF_L_MFP &= 0x00007700;

		OFF_ADC_POW;
		OFF_INDI_POW;
		
		OFF1_PWEN;
		OFF2_PWEN;
		OFF3_PWEN;
		OFF4_PWEN;
		
		OFF_D1_POW;
		OFF_D2_POW;
		OFF_D3_POW;
		OFF_D4_POW;

    CLK->APBCLK &= ~CLK_APBCLK_LCD_EN; /* Disable LCD clock */
    CLK->PWRCTL |= CLK_PWRCTL_LXT_EN_Msk; /* enable LXT - 32Khz */
    CLK->PWRCTL |= CLK_PWRCTL_PD_WK_IE_Msk;  /* Enable wake up interrupt source */
    NVIC_EnableIRQ(PDWU_IRQn);             /* Enable IRQ request for PDWU interupt */
    CLK_PowerDown();
}


void GPABC_IRQHandler(void)
{
    /* To check if PA.8 interrupt occurred */
    if (PA->ISRC & BIT8) 
		{
        PA->ISRC = BIT8;
				flag_wakeup = 1;
				flag_con_discon=1;
			
//				SYS_Init();
//	
//				GPIO_Init();
//			
//				MX_SPI1_Init();
//			
//				ADC_Init();
//			
//				Timer0_Init();
//				Timer1_Init();
//				/* Init UART to 115200-8n1 for print message */
//				UART_Open(UART1, 9600);
			
#ifdef DEBUG
		printf("PA.8 INT occurred. \n");
#endif				
        
    }
		else if(PB->ISRC & BIT14)
		{
				PB->ISRC = BIT14;
				flag_wakeup = 0;
				flag_con_discon=1;
#ifdef DEBUG
		printf("PB.14 INT occurred. \n");
#endif	
        
		}
		else {
        /* Un-expected interrupt. Just clear all PORTA, PORTB, PORTC interrupts */
        PA->ISRC = PA->ISRC;
        PB->ISRC = PB->ISRC;
        PC->ISRC = PC->ISRC;
        //printf("Un-expected interrupts. \n");
    }
}

void Timer1_Init()
{
	  // Set timer frequency to 100Hz,10ms
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 100);

    // Enable timer interrupt
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);
    // Start Timer 0
    TIMER_Start(TIMER1);
}

void TMR1_IRQHandler(void)
{
			TIMER_ClearIntFlag(TIMER1);
	
			if(flag_detect % 50 == 0)
			{
				if(Self_test_flag&8)
				{
					F_N_LED_BLE;
				}
				if(flag_poweron>500 && p_battery[0]<3400)
				{
					F_N_LED_BAT;
				}
			}
			flag_detect++;
			
			if(flag_detect >= 60000)
			{
				flag_detect = 0;
			}
			
			if(flag_poweron < 500)
			{
				flag_poweron++;				
				if(flag_500ms_tick%50 == 0)
				{
					flag_500ms_tick=0;
					if(p_battery[0]>3400)
					{
						ON_LED_BAT;
					}
					else
					{
						F_N_LED_BAT;
					}
				}
				flag_500ms_tick++;
			}
			else if(flag_poweron==500)
			{
				
				OFF_LED_BLE;
				OFF_LED_BAT;
				flag_wakeup=0;
				flag_poweron++;
			}		

}


void select_usart(uint8_t a,uint8_t b,uint8_t c,uint8_t d)
{	
	ON_LS_EN_A_ON;
	ON_LS_EN_B_ON;
	if(a)
	{
		LS_SEL0_Pin = 1;
	}
	else
	{
		LS_SEL0_Pin = 0;
	}
	
	if(b)
	{
		LS_SEL1_Pin = 1;
	}
	else
	{
		LS_SEL1_Pin = 0;
	}
	
	if(c)
	{
		LS_SEL2_Pin = 1;
	}
	else
	{
		LS_SEL2_Pin = 0;
	}
	
	if(d)
	{
		LS_SEL3_Pin = 1;
	}
	else
	{
		LS_SEL3_Pin = 0;
	}

}


uint16_t usart_receive_1_16_ctrl(uint8_t code)
{
	//ON_LS_EN_OFF;
	switch(code)
	{
		case 0:
					select_usart(0,0,0,0);
					break;
		case 1:
					select_usart(1,0,0,0);
					break;
		case 2:
					select_usart(0,1,0,0);
					break;
		case 3:
					select_usart(1,1,0,0);
					break;
		case 4:
					select_usart(0,0,1,0);
					break;
		case 5:
					select_usart(1,0,1,0);
					break;
		case 6:
					select_usart(0,1,1,0);
					break;
		case 7:
					select_usart(1,1,1,0);
					break;
		case 8:
					select_usart(0,0,0,1);
					break;
		case 9:
					select_usart(1,0,0,1);
					break;																									
  	case 10:
  				select_usart(0,1,0,1);
					break;
		case 11:
					select_usart(1,1,0,1);
					break;
		case 12:
					select_usart(0,0,1,1);
					break;
		case 13:
					select_usart(1,0,1,1);
					break;
		case 14:
					select_usart(0,1,1,1);
					break;
		case 15:
					select_usart(1,1,1,1);
					break;
		default	:
		         	break;

	}
	//ON_LS_EN_OK;
	return 1;
}

uint16_t usart_send_1_4_ctrl(uint8_t code)
{
	switch(code)
	{
		case 0:
					usart_receive_1_16_ctrl(0);
					break;
		case 1:
					usart_receive_1_16_ctrl(4);
					break;
		case 2:
					usart_receive_1_16_ctrl(8);
					break;
		case 3:
					usart_receive_1_16_ctrl(12);
					break;
		default	:
		      break;
	}
	return 1;
}

uint8_t power_on(uint8_t num)
{
	if(num==0)
	{
		ON1_PWEN;
	}
	else if(num==1)
	{
		ON2_PWEN;
	}
	else if(num==2)
	{
		ON3_PWEN;
	}
	else if(num==3)
	{
		ON4_PWEN;
	}
	return 1;
}
uint8_t power_off(uint8_t num)
{
	if(num==0)
	{
		OFF1_PWEN;
	}
	else if(num==1)
	{
		OFF2_PWEN;
	}
	else if(num==2)
	{
		OFF3_PWEN;
	}
	else if(num==3)
	{
		OFF4_PWEN;
	}
	return 1;
}

void error_handle()
{
	ON_SPK;
	Delay(300);
	OFF_SPK;
	Delay(100);
	ON_SPK;
	Delay(300);
	OFF_SPK;
	Delay(100);
	ON_SPK;
	Delay(300);
	OFF_SPK;
}

void start_handle()
{
	ON_SPK;
	Delay(200);
	OFF_SPK;
}

void end_handle()
{
	ON_SPK;
	Delay(100);
	OFF_SPK;
	Delay(100);
	ON_SPK;
	Delay(100);
	OFF_SPK;
}

