#include "user_usart.h"
#include <stdio.h>
#include "Nano100Series.h"



void SENSOR_usart_RX_Clear(void);
void BLE_usart_RX_Clear(void);

uint8_t flag_BLE_OK = 0;
uint8_t flag_BLE_newline = 0;
uint8_t flag_BLE_CMD =0;
USART_struct SENSOR_usart;
USART_struct BLE_usart;
USART_struct SENSOR_usart={
	                          .reset=&SENSOR_usart_RX_Clear,
													};

USART_struct BLE_usart={
	                           .reset=&BLE_usart_RX_Clear,
												};

// USART0 接收BUF清零
void SENSOR_usart_RX_Clear(void)                	//串口1中断服务程序
{
	 uint8_t i;
   NVIC_DisableIRQ(UART0_IRQn);//UE  禁止UART
   for( i = 0 ; i < SIZE_LEN ; i ++)
	 {
		 SENSOR_usart.receivebuf[i]=0;
	 }
	 NVIC_EnableIRQ(UART0_IRQn);//UE  使能UART
	 SENSOR_usart.flag_complete=0;
	 SENSOR_usart.receive_cnt=0;
}
// USART1 接收BUF清零
void BLE_usart_RX_Clear(void)
{
	 uint8_t i;
   NVIC_DisableIRQ(UART1_IRQn);//UE  禁止UART
   for( i = 0 ; i < SIZE_LEN ; i ++)
	 {
		 BLE_usart.receivebuf[i]=0;
	 }
	 NVIC_EnableIRQ(UART1_IRQn);//UE  使能UART
	 BLE_usart.flag_complete=0;
	 BLE_usart.receive_cnt=0;
}
												
void UART0_Init()
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    SENSOR_usart.reset = &SENSOR_usart_RX_Clear;
		UART_Open(UART0, 9600);
		UART_ENABLE_INT(UART0, (UART_IER_RDA_IE_Msk | UART_IER_RTO_IE_Msk));
    NVIC_EnableIRQ(UART0_IRQn);
}

void UART1_Init()
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
		BLE_usart.reset=&BLE_usart_RX_Clear;
    UART_Open(UART1, 115200);
		UART_ENABLE_INT(UART1, (UART_IER_RDA_IE_Msk | UART_IER_RTO_IE_Msk));
    NVIC_EnableIRQ(UART1_IRQn);
}

void Timer0_Init(void)
{
	    // Set timer frequency to 1000Hz,1ms
    TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 1000);

    // Enable timer interrupt
    TIMER_EnableInt(TIMER0);
    NVIC_EnableIRQ(TMR0_IRQn);

    // Start Timer 0
    TIMER_Start(TIMER0);
	
}


/*---------------------------------------------------------------------------------------------------------*/
/* UART Callback function                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
void UART0_HANDLE()
{
		uint8_t u8InChar=0xFF;
    uint32_t u32IntSts= UART0->ISR;

    if(u32IntSts & UART_ISR_RDA_IS_Msk) 
		{
				while(!UART_GET_RX_EMPTY(UART0)) 
				{
						SENSOR_usart.tim_cnt=0;
						SENSOR_usart.flag_complete =0;
						SENSOR_usart.flag_update=1;
						SENSOR_usart.receive_cnt++;
							
						if(SENSOR_usart.receive_cnt>=SIZE_LEN)
						{
							SENSOR_usart.receive_cnt=SIZE_LEN;
						}
						u8InChar = UART_READ(UART0);
						SENSOR_usart.receivebuf[SENSOR_usart.receive_cnt-1] = u8InChar;
        }
			
		}
	
}
void UART1_HANDLE()
{
	  uint8_t u8InChar=0xFF;
    uint32_t u32IntSts= UART1->ISR;

    if(u32IntSts & UART_ISR_RDA_IS_Msk) 
		{
//        printf("\nInput:");
        /* Get all the input characters */
        while(!UART_GET_RX_EMPTY(UART1)) 
				{
						BLE_usart.tim_cnt=0;
						
						BLE_usart.flag_update=1;
						BLE_usart.receive_cnt++;
						BLE_usart.flag_complete=0;
						if(BLE_usart.receive_cnt>=SIZE_LEN)
						{
							BLE_usart.receive_cnt=SIZE_LEN;
						}
						u8InChar = UART_READ(UART1);
						BLE_usart.receivebuf[BLE_usart.receive_cnt-1] = u8InChar;//接收到的数据保存到缓存数组
        }
//        printf("\nTransmission Test:");
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* ISR to handle UART Channel 0 interrupt event                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void UART0_IRQHandler(void)
{
    UART0_HANDLE();
}

/*---------------------------------------------------------------------------------------------------------*/
/* ISR to handle UART Channel 0 interrupt event                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void UART1_IRQHandler(void)
{
    UART1_HANDLE();
}

void SENSOR_usart_service(void)//
{
	if(1 == SENSOR_usart.flag_update)
	{
		SENSOR_usart.flag_lock=1;
		if(SENSOR_usart.tim_cnt>10)
		{
			SENSOR_usart.tim_cnt=0;
			SENSOR_usart.flag_update=0;
			SENSOR_usart.flag_complete =1;
		}
	}
	else
	{
		if(1 == SENSOR_usart.flag_lock)
		{
				SENSOR_usart.flag_lock=0;
		}
	}
}

void BLE_usart_service(void)//
{
	if(1 == BLE_usart.flag_update)
	{
		BLE_usart.flag_lock = 1;
		if(BLE_usart.tim_cnt > 50)
		{
			BLE_usart.tim_cnt=0;
			BLE_usart.flag_update=0;
			BLE_usart.flag_complete =1;
		}
	}
	else
	{
		if(1 == BLE_usart.flag_lock)
		{
				BLE_usart.flag_lock=0;
				if(BLE_usart.receive_cnt>=4)
				{
					if((BLE_usart.receivebuf[0] == '+' && BLE_usart.receivebuf[1] == 'W'&&BLE_usart.receivebuf[2] == 'A')||
					(BLE_usart.receivebuf[0] == 'O'&& BLE_usart.receivebuf[1] == 'K'))
					{
							flag_BLE_OK = 1;
					}
				}
				if(BLE_usart.receive_cnt >= 2)
				{
						if(((BLE_usart.receivebuf[BLE_usart.receive_cnt-2]) == 0x1d)&&(BLE_usart.receivebuf[BLE_usart.receive_cnt-1] == 0x0d))
						{
							//flag2 = 1;
							flag_BLE_CMD = 1;
						}
						if(((BLE_usart.receivebuf[BLE_usart.receive_cnt-2]) == 0x0d)&&(BLE_usart.receivebuf[BLE_usart.receive_cnt-1] == 0x0a))
						{
							flag_BLE_newline = 1;
						}
				}
		}
	}
}

void TMR0_IRQHandler(void)
{ 
		TIMER_ClearIntFlag(TIMER0);
	
		//SENSOR_usart_service();
		BLE_usart_service();
		if(SENSOR_usart.tim_cnt > 60000)
		{
			SENSOR_usart.tim_cnt = 0;
		}
		if(BLE_usart.tim_cnt > 60000)
		{
			BLE_usart.tim_cnt = 0;
		}
		SENSOR_usart.tim_cnt++;
		BLE_usart.tim_cnt++;

}

