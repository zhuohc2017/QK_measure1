#include "user_spi.h"

void MX_SPI1_Init(void)
{
		/* Configure SPI0 as a master, MSB first, 8-bit transaction, SPI Mode-0 timing, clock is 2MHz */
    SPI_Open(SPI1, SPI_MASTER, SPI_MODE_0, 8, 1000000);
	
		SPI_SET_MSB_FIRST(SPI1);
	
	  /* Enable the automatic hardware slave select function. Select the SPI0_SS0 pin and configure as low-active. */
    SPI_DisableAutoSS(SPI1);

    /* Enable SPI0 two bit mode */
    SPI_DISABLE_2BIT_MODE(SPI1);

}


//发送字节
uint8_t SPI_SendByte(uint8_t byte)
{
	uint8_t temp =0xf0;	
	
	SPI_WRITE_TX0(SPI1,byte);
	
	SPI_TRIGGER(SPI1);
	
	while(SPI_IS_BUSY(SPI1))
	{
		
	}
	
	temp = SPI_READ_RX0(SPI1);  
	
	return temp;
}

//读取字节
uint8_t SPI_Flash_ReadByte(void)
{
	uint8_t temp = 0xf0;
	
	SPI_WRITE_TX0(SPI1,0xFF);
	
	SPI_TRIGGER(SPI1);
	
	while(SPI_IS_BUSY(SPI1))
	{
		
	}
	
	temp = SPI_READ_RX0(SPI1);  
	
	return temp;
}





