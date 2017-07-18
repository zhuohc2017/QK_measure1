#ifndef W25Q64_H
#define W25Q64_H

#include "user_spi.h"

//W25Q64BV
#define W25Q64_DeviceID 0xEF16
 
#define W25X_CMD_WriteEnable 0x06 /*Write enable instruction */
#define W25X_CMD_WriteDisable 0x04 /*! Write to Memory Disable */
#define W25X_CMD_WriteStatusReg 0x01 /* Write Status Register instruction */
 
#define W25X_CMD_PageProgram 0x02 /* Write enable instruction */
#define W25X_CMD_QuadPageProgram 0x32 /* Write enable instruction */
 
#define W25X_CMD_BlockErase64 0xD8 /* Block 64k Erase instruction */
#define W25X_CMD_BlockErase32 0x52 /* Block 32k Erase instruction */
#define W25X_CMD_ChipErase 0xC7 /* Bulk Erase instruction */
#define W25X_CMD_SectorErase 0x20 /* Sector 4k Erase instruction */
#define W25X_CMD_EraseSuspend 0x75 /* Sector 4k Erase instruction */
#define W25X_CMD_EraseResume 0x7a /* Sector 4k Erase instruction */
 
#define W25X_CMD_ReadStatusReg1 0x05 /* Read Status Register instruction */
#define W25X_CMD_ReadStatusReg2 0x35 /* Read Status Register instruction */
 
#define W25X_CMD_High_Perform_Mode 0xa3
#define W25X_CMD_Conti_Read_Mode_Ret 0xff
 
#define W25X_WakeUp 0xAB
#define W25X_JedecDeviveID 0x9F /*Read identification */
#define W25X_ManufactDeviveID 0x90 /* Read identification */
#define W25X_ReadUniqueID 0x4B
 
#define W25X_Power_Down 0xB9 /*Sector 4k Erase instruction */
 
#define W25X_CMD_ReadData 0x03 /* Read from Memory instruction */
#define W25X_CMD_FastRead 0x0b /* Read from Memory instruction */
#define W25X_CMD_FastReadDualOut 0x3b /*Read from Memory instruction */
#define W25X_CMD_FastReadDualIO 0xBB /* Read from Memory instruction */
#define W25X_CMD_FastReadQuadOut 0x6b /* Read from Memory instruction */
#define W25X_CMD_FastReadQuadIO 0xeb /* Read from Memory instruction */
#define W25X_CMD_OctalWordRead 0xe3 /* Read from Memory instruction */
#define W25X_ReleasePowerDown           0xAB  
#define W25X_DUMMY_BYTE 0xff //0xA5
#define W25X_SPI_PAGESIZE 0x100
#define Dummy_Byte                      0xA5
#define SPI_FLASH_PageSize              256
#define SPI_FLASH_PerWritePageSize      256
#define WIP_FlagMask                    0x01  /* Write In Progress (WIP) flag */
#define W25X_FLASH_SPI SPI1

void SPI_Flash_WAKEUP(void);
uint16_t SPI_Flash_ReadID(void);
void SPI_Flash_Write_Enable(void); 
void SPI_FLASH_Write_Disable(void);

uint8_t SPI_Flash_ReadByte(void);
uint8_t SPI_Flash_SendByte(uint8_t byte);
void SPI_Flash_WaitForWriteEnd(void);
uint8_t SPI_Flash_ReadSR(void);

void SPI_Flash_ReadBuffer(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
void SPI_FLASH_PageWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void SPI_Flash_Writebuffer(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void SPI_Flash_EraseChip(uint32_t write_addr);
void W25QXX_Erase_Sector(uint32_t Dst_Addr);

#endif


