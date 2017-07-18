/**********************************************************
SPI FLASH 共2048（0x800）个扇区
7FF扇区即最后一个扇区用于临时数据存储
7FF扇区即倒数第二个扇区用于存取测距传感器ID
 1---- 0- 5字节
 2---- 6-11字节
 3----12-17字节
 ......
16----90-95字节

**********************************************************/
#include "w25q64.h"

#define START_PAGE   0x0f
#define ADDR_FLAG    0x00

#define  W25X_FLASH_CS_HIGH()  SPI_SET_SS0_HIGH(SPI1)								//SPI片选脚拉高

#define  W25X_FLASH_CS_LOW()   SPI_SET_SS0_LOW(SPI1)							//SPI片选脚拉低



uint16_t MEASURE_TIMEOUT=2500;
//读取设备ID

uint16_t SPI_Flash_ReadID(void)
{
    uint32_t Temp = 0, Temp1 = 0, Temp2 = 0;
    /*à-μíSPI_FLASH????D?o???*/
    W25X_FLASH_CS_LOW();

    /* ×??-?áè?ID ê±Dò￡?·￠?í?üá?*/
    SPI_SendByte(W25X_ManufactDeviveID);
	  SPI_SendByte(0x00);
    SPI_SendByte(0x00);
    SPI_SendByte(0x00);
    //Temp0 = SPI_SendByte(W25X_DUMMY_BYTE);
    Temp1 = SPI_SendByte(W25X_DUMMY_BYTE);
    Temp2 = SPI_SendByte(W25X_DUMMY_BYTE);
     /*à-??SPI_FLASH????D?o???*/
    W25X_FLASH_CS_HIGH();
    //(Temp0 << 16)|
    Temp = (Temp1 << 8)|Temp2;
    return Temp;
}
//写一页256字节内。
void SPI_Flash_Writebuffer(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
		uint8_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;
    Addr = WriteAddr % SPI_FLASH_PageSize;
    count = SPI_FLASH_PageSize - Addr;
    NumOfPage =  NumByteToWrite / SPI_FLASH_PageSize;
    NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;

    if (Addr == 0) /* WriteAddr is SPI_FLASH_PageSize aligned  */
    {
        if (NumOfPage == 0) /* NumByteToWrite < SPI_FLASH_PageSize */
        {
            SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumByteToWrite);
        }
        else /* NumByteToWrite > SPI_FLASH_PageSize */
        {
            while(NumOfPage--)
            {
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, SPI_FLASH_PageSize);
                WriteAddr +=  SPI_FLASH_PageSize;
                pBuffer += SPI_FLASH_PageSize;
            }

            SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumOfSingle);
        }
    }
    else /* WriteAddr is not SPI_FLASH_PageSize aligned  */
    {
        if (NumOfPage == 0) /* NumByteToWrite < SPI_FLASH_PageSize */
        {
            if (NumOfSingle > count) /* (NumByteToWrite + WriteAddr) > SPI_FLASH_PageSize */
            {
                temp = NumOfSingle - count;

                SPI_FLASH_PageWrite(pBuffer, WriteAddr, count);
                WriteAddr +=  count;
                pBuffer += count;

                SPI_FLASH_PageWrite(pBuffer, WriteAddr, temp);
            }
            else
            {
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumByteToWrite);
            }
        }
        else /* NumByteToWrite > SPI_FLASH_PageSize */
        {
            NumByteToWrite -= count;
            NumOfPage =  NumByteToWrite / SPI_FLASH_PageSize;
            NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;

            SPI_FLASH_PageWrite(pBuffer, WriteAddr, count);
            WriteAddr +=  count;
            pBuffer += count;

            while (NumOfPage--)
            {
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, SPI_FLASH_PageSize);
                WriteAddr +=  SPI_FLASH_PageSize;
                pBuffer += SPI_FLASH_PageSize;
            }
            if (NumOfSingle != 0)
            {
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumOfSingle);
            }
        }
    }
}

//读
void SPI_Flash_ReadBuffer(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{
/*Select the FLASH: Chip Select low */
W25X_FLASH_CS_LOW();

/*Send &quot;Read from Memory &quot; instruction */
SPI_SendByte(W25X_CMD_ReadData);

/*!&lt; Send ReadAddr high nibble address byte to read from */
SPI_SendByte((ReadAddr & 0xFF0000)>>16);
/*!&lt; Send ReadAddr medium nibble address byte to read from */
SPI_SendByte((ReadAddr& 0xFF00)>>8);
/*!&lt; Send ReadAddr low nibble address byte to read from */
SPI_SendByte(ReadAddr & 0xFF);

while (NumByteToRead--) /* while there is data to be read */
{
/*!&lt; Read a byte from the FLASH */
*pBuffer = SPI_SendByte(W25X_DUMMY_BYTE);
/*!&lt; Point to the next location where the byte read will be saved */
pBuffer++;
}

/*!&lt; Deselect the FLASH: Chip Select high */
W25X_FLASH_CS_HIGH();
}

void SPI_Flash_EraseChip(uint32_t write_addr)
{
	/*!&lt; Send write enable instruction */
	SPI_Flash_Write_Enable();

	/*!&lt; Bulk Erase */
	/*!&lt; Select the FLASH: Chip Select low */
	W25X_FLASH_CS_LOW();
	/*!&lt; Send Bulk Erase instruction */
	SPI_SendByte(write_addr);
	/*!&lt; Deselect the FLASH: Chip Select high */
	W25X_FLASH_CS_HIGH();

	/*!&lt; Wait the end of Flash writing */
	SPI_Flash_WaitForWriteEnd();
}

void W25QXX_Erase_Sector(uint32_t Dst_Addr)
{
 	  Dst_Addr*=4096;
    SPI_Flash_Write_Enable();                  	//SET WEL
    SPI_Flash_WaitForWriteEnd();
  	W25X_FLASH_CS_LOW();                            	//ê1?ü?÷?t
    SPI_SendByte(W25X_CMD_SectorErase);      	//·￠?íéè??2á3y??á?
    SPI_SendByte((uint8_t)((Dst_Addr)>>16));  	//·￠?í24bitμ??·
    SPI_SendByte((uint8_t)((Dst_Addr)>>8));
    SPI_SendByte((uint8_t)Dst_Addr);
	  W25X_FLASH_CS_HIGH();                            	//è???????
    SPI_Flash_WaitForWriteEnd();   				   		//μè′y2á3yíê3é
}

/**-----------------------------------------------------------------
  * @oˉêy?? SPI_Flash_WAKEUP
  * @1|?ü   ??D?SPI FLASH
  * @2?êy   ?T
  * @·μ???μ ?T
***----------------------------------------------------------------*/
void SPI_Flash_WAKEUP(void)
{
  /* Select the FLASH: Chip Select low */
  W25X_FLASH_CS_LOW();

  /* Send "Power Down" instruction */
  SPI_SendByte(W25X_ReleasePowerDown);

  /* Deselect the FLASH: Chip Select high */
  W25X_FLASH_CS_HIGH();
}

void SPI_FLASH_PageWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    /* Enable the write access to the FLASH */
    SPI_Flash_Write_Enable();

    /* Select the FLASH: Chip Select low */
    W25X_FLASH_CS_LOW();
    /* Send "Write to Memory " instruction */
    SPI_SendByte(W25X_CMD_PageProgram);
    /* Send WriteAddr high nibble address byte to write to */
    SPI_SendByte((WriteAddr & 0xFF0000) >> 16);
    /* Send WriteAddr medium nibble address byte to write to */
    SPI_SendByte((WriteAddr & 0xFF00) >> 8);
    /* Send WriteAddr low nibble address byte to write to */
    SPI_SendByte(WriteAddr & 0xFF);

    if(NumByteToWrite > SPI_FLASH_PerWritePageSize)
    {
        NumByteToWrite = SPI_FLASH_PerWritePageSize;
    }

    /* while there is data to be written on the FLASH */
    while (NumByteToWrite--)
    {
        /* Send the current byte */
        SPI_SendByte(*pBuffer);
        /* Point on the next byte to be written */
        pBuffer++;
    }

    /* Deselect the FLASH: Chip Select high */
    W25X_FLASH_CS_HIGH();

    /* Wait the end of Flash writing */
    SPI_Flash_WaitForWriteEnd();
}


void SPI_Flash_Write_Enable(void)
{
	W25X_FLASH_CS_LOW();
	SPI_SendByte(W25X_CMD_WriteEnable);
	W25X_FLASH_CS_HIGH();
}

void SPI_FLASH_Write_Disable(void)
{
	W25X_FLASH_CS_LOW();
	SPI_SendByte(W25X_CMD_WriteDisable);
	W25X_FLASH_CS_HIGH();
}

void SPI_Flash_WaitForWriteEnd(void)
{
//	uint8_t FLASH_Status = 0;

  /* Select the FLASH: Chip Select low */
  W25X_FLASH_CS_LOW();

  /* Send "Read Status Register" instruction */
  SPI_SendByte(W25X_CMD_ReadStatusReg1);

  /* Loop as long as the memory is busy with a write cycle */
//  do
//  {
//    /* Send a dummy byte to generate the clock needed by the FLASH
//    and put the value of the status register in FLASH_Status variable */
//    FLASH_Status = SPI_SendByte(Dummy_Byte);

//  }
//  while ((FLASH_Status & WIP_FlagMask) == SET); /* Write in progress */

  /* Deselect the FLASH: Chip Select high */
  W25X_FLASH_CS_HIGH();
}

uint8_t SPI_Flash_ReadSR(void)
{
    uint8_t byte=0;

    W25X_FLASH_CS_LOW();
    SPI_SendByte(W25X_CMD_ReadStatusReg1);
    byte=SPI_SendByte(0Xff);
    W25X_FLASH_CS_HIGH();
    return byte;
}


//uint8_t searchcount()
//{
//	 uint32_t he_num = 191;
//	 uint8_t  log_qk[18]={0};
//	 uint16_t total_count = 0;
//   while(1)
//	 {
//			   SPI_Flash_ReadBuffer(log_qk,he_num,1);
//				 he_num = he_num + 0x100;
//				 if(log_qk[0] == 0x0d)
//				 {
//					 total_count = total_count + 1;
//					 if(total_count >= 0x7FE0)
//					 {

//						 break;
//					 }
//				 }
//				 else if(log_qk[0] == 0xff)
//				 {
//						break;
//				 }
//				 else
//				 {
//						break;
//				 }
//		}
//	 return total_count;
//}

//uint8_t searchData(uint8_t *data,uint32_t *index,uint32_t *total)
//{
//	 uint32_t he_num = 191;
//	 uint8_t  log_qk[18]={0};
//	 uint32_t index_count = 0;
//	 uint32_t total_count = 0;
//	 uint8_t i=0,equeal_flag = 1;//计数器，相等标志
//	 uint8_t search_flag=1;
//   while(1)
//	 {
//			   SPI_Flash_ReadBuffer(log_qk,he_num,1);
//				 if(log_qk[0] == 0x0d)
//				 {
//					 SPI_Flash_ReadBuffer(log_qk,he_num-191,18);
//					 if(search_flag==1)
//					 {
//						 equeal_flag=1;
//						 for(i=3;i<18;i++)         //查询单元信息是否相等
//						 {
//							 if(data[i]!=log_qk[i])  //有一个不相等就是不同，不同继续查找下一组数据
//							 {
//								 equeal_flag = 0;
//								 index_count = index_count+1;
//								 break;
//							 }
//						 }
//						 if(equeal_flag==1)      //如果相等，查询到，返回index
//						 {
//							 search_flag = 0;
//							 *index = index_count+1;
//						 }
//				   }

//					 total_count = total_count + 1;
//					 if(total_count >= 0x7FE0)
//					 {
//						 break;
//					 }
//				 }
//				 else if(log_qk[0] == 0xff)
//				 {
////						if((he_num-185) == 0)
////						{
////							 SPI_Flash_EraseChip(W25X_CMD_ChipErase);
////						}
//						break;
//				 }
//				 else
//				 {
//						break;
//				 }
//				 he_num = he_num + 0x100;
//		}
//		 *total = total_count;
//		 if(total_count == index_count)//如果查询到结尾也没查询到，则返回0xFF00；
//		 {
//			 *index=0xFF00;
//			 return 0;
//		 }
//		 else
//		 {
//			 return 1;
//		 }
//}

//uint8_t  write_format_control(uint8_t * data)
//{
//		W25QXX_Erase_Sector(0);
//	SPI_Flash_Writebuffer(data,0,180);
//	return 0;
//}


