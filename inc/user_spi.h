#ifndef __USER_SPI_H__
#define __USER_SPI_H__
#include "Nano100Series.h"

uint8_t SPI_SendByte(uint8_t byte);
uint8_t SPI_Flash_ReadByte(void);
uint16_t SPI_I2S_ReceiveData(void);
void SPI_I2S_SendData(uint16_t Data);
void MX_SPI1_Init(void);

#endif

