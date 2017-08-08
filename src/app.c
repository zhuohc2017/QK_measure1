#include "app.h"
#include "string.h"
#define self_test_power_on_delay 800
extern USART_struct BLE_usart;
extern USART_struct SENSOR_usart;

extern uint16_t MEASURE_TIMEOUT;
extern uint16_t p_battery[5];

extern uint8_t flag_BLE_OK;//蓝牙检测标志
extern uint8_t flag_BLE_newline;
extern uint8_t flag_BLE_CMD;
extern uint16_t wakeup_cnt;
uint8_t  Self_test_flag = 0;
uint16_t Self_sensor_flag = 0;

uint8_t ERR_Com[32];//传感器测量误差补偿
uint8_t IR_co[8];
void Sensorwrite(uint8_t *Txbuffer,uint8_t len) 
{
	UART_Write(UART0,Txbuffer,len);
}

void Blewrite(uint8_t *Txbuffer,uint8_t len) 
{
	UART_Write(UART1,Txbuffer,len);
}

void Execute_Cmd(uint8_t *cmd)
{
	//HAL_NVIC_EnableIRQ(TIM2_IRQn);
	if(BLE_usart.receivebuf[BLE_usart.receive_cnt-1]!=0x0d||BLE_usart.receivebuf[BLE_usart.receive_cnt-2]!=0x1d)//如果结束符不是1d 0d，则报错
	{
		error_handle();
	}
	switch(cmd[0])
	{
		case 0x01://01 xx 1d 0d 读红外传感器电压电压值；
		{
			uint16_t p_battery[5]={0};
			uint8_t rtn[8]={0};
			uint8_t No = BLE_usart.receivebuf[1];
		
			if(BLE_usart.receivebuf[1]>4|| BLE_usart.receivebuf[1]<1)
			{
				error_handle();
					break;
			}
			else
			{
				start_handle();
				IRsensor_ADC_convert(p_battery);
				rtn[0] = 0x01;
				rtn[1] = No;
				rtn[2] = p_battery[No]/100+0x30;
				rtn[3] = '.';
				rtn[4] = p_battery[No]%100/10+0x30;
				rtn[5] = p_battery[No]%10+0x30;
				rtn[6] = 'V';
				rtn[7] = 0x24;
				Delay(1000);
				Blewrite(rtn,8);
				Delay(500);
				BLE_usart.reset();
				end_handle();
			}
		}
		break;
		case 0x02:  //误差补偿
		{
			uint8_t i = 0,rtn[2]={0};
			if(BLE_usart.receivebuf[4] != 0x1d||BLE_usart.receivebuf[5] != 0x0d)
			{
				for(i = 0;i < SIZE_LEN;i++)
				{
						BLE_usart.receivebuf[i] =  0;
				}
				error_handle();
				break;
			}
			 start_handle();
			 Delay(1000);
			 set_IR((uint8_t*)&BLE_usart.receivebuf[1]);
			 for(i = 0;i<SIZE_LEN;i++)
			 {
				 BLE_usart.receivebuf[i] = 0;
			 }
			 rtn[0] = 0x02;
			 rtn[1] = 0x24;
			 Blewrite(rtn,2);
			 end_handle();
		}
		break;
		case 0x03:  //调试用
		{
			uint8_t i = 0,rtn[2]={0};
			if(BLE_usart.receivebuf[1] != 0x1d||BLE_usart.receivebuf[2] != 0x0d)
			{
				for(i = 0;i < SIZE_LEN;i++)
				{
						BLE_usart.receivebuf[i] =  0;
				}
				error_handle();
				break;
			}
			 start_handle();
			 Delay(1000);
			 ON_ADC_POW;
			 ON_D1_POW;
			 ON_D2_POW;
			 ON_D3_POW;
			 ON_D4_POW;
			 rtn[0] = 0x03;
			 rtn[1] = 0x24;
			 Blewrite(rtn,2);
			 end_handle();
		}
		break;
		case 0x11:
		{
				uint8_t num_of_sensor = BLE_usart.receivebuf[1];   //蓝牙接收到的传感器编号
				uint8_t cmd_one_measure[4] ={0x80,0x06,02,0x78};
				uint16_t time_out = 0;
				uint8_t  IR_cnt=0;
				uint32_t result_measure = 0;
				if(num_of_sensor > 16||num_of_sensor < 1)
				{
					error_handle();
					break;
				}
				start_handle();
				power_on((num_of_sensor-1)/4);
				Delay(500);
				usart_receive_1_16_ctrl(num_of_sensor-1);//切换到对应传感器接收
				Delay(100);
				SENSOR_usart.reset();                   //清buf计数
				Delay(10);
				Sensorwrite(cmd_one_measure,4);
				Delay(500);
				time_out = 0;
				while(time_out < 4000)
				{
					SENSOR_usart_service();
					if(SENSOR_usart.flag_complete==1)
					{
						if(SENSOR_usart.receive_cnt == 12)
						{
							time_out = 0;
							break;
						}
						else if(SENSOR_usart.receive_cnt == 0x0d)//遇到过最前面多一个0x00
						{
								if(SENSOR_usart.receivebuf[2] == 0x06&&SENSOR_usart.receivebuf[3]==0x82)
								{
									for(uint8_t i=0;i<7;i++)
									{
										SENSOR_usart.receivebuf[4+i]=SENSOR_usart.receivebuf[5+i];
									}
								}
								break;
						}
						
					}
//					if((time_out != 0) && (time_out%50 == 0))
//					{
//						SENSOR_usart.receive_cnt = 0;                   //清buf计数
//						usart_send_buffer(&huart1,cmd_one_measure,4);
//						Delay(20);
//					}
					time_out++;
					Delay(2);
				}
				power_off((num_of_sensor-1)/4);
			
				uint8_t result_buf[20]={0x11};
				result_buf[1] = num_of_sensor/10+0x30;//把传感器序号转换成字符
				result_buf[2] = num_of_sensor%10+0x30;
				uint32_t m_distance[5]={300,300,300,300,300};
				if(num_of_sensor==2||num_of_sensor==6||num_of_sensor==9||num_of_sensor==13)
				{
					if(num_of_sensor==2)
					{
						IR_cnt=1;
					}
					else if(num_of_sensor==6)
					{
						IR_cnt=2;
					}
					else if(num_of_sensor==9)
					{
						IR_cnt=3;
					}
					else if(num_of_sensor==13)
					{
						IR_cnt=4;
					}
					ADC_convert(m_distance);
					
					if(m_distance[IR_cnt]>35)
					{
						m_distance[IR_cnt]=300;
					}
				}

				if(SENSOR_usart.receivebuf[7]==0x2E)
				{
					result_measure = (SENSOR_usart.receivebuf[4]-0x30)*100000+(SENSOR_usart.receivebuf[5]-0x30)*10000
															+(SENSOR_usart.receivebuf[6]-0x30)*1000+(SENSOR_usart.receivebuf[8]-0x30)*100
							                +(SENSOR_usart.receivebuf[9]-0x30)*10+SENSOR_usart.receivebuf[10]-0x30;
					if(ERR_Com[2*(num_of_sensor-1)]=='+')
					{
						result_measure += ERR_Com[2*(num_of_sensor-1)+1];
//						double f_temp=0.0;
//						char str_temp[7]={0};
						
//						f_temp=result_measure/1000.0;
//						sprintf(str_temp,"%.7f",f_temp);
//						strncpy(&USART1_RX_BUF[4],str_temp,7);
						if(result_measure>15&&result_measure<50)
						{
							result_measure-=10;
						}
						if(result_measure<40&&m_distance[IR_cnt]<36)
						{
							result_measure = m_distance[IR_cnt];
						}

						SENSOR_usart.receivebuf[4] = result_measure/100000+0x30;
						SENSOR_usart.receivebuf[5] = result_measure%100000/10000+0x30;
						SENSOR_usart.receivebuf[6] = result_measure%10000/1000+0x30;
						SENSOR_usart.receivebuf[8] = result_measure%1000/100+0x30;
						SENSOR_usart.receivebuf[9] = result_measure%100/10+0x30;
						SENSOR_usart.receivebuf[10] = result_measure%10+0x30;
					}
					else if(ERR_Com[2*(num_of_sensor-1)]=='-')
					{
						if(result_measure >= ERR_Com[2*(num_of_sensor-1)+1])
						{
								result_measure -= ERR_Com[2*(num_of_sensor-1)+1];
								if(result_measure>15&&result_measure<50)
								{
									result_measure-=10;
								}
								if(result_measure<40&&m_distance[IR_cnt]>0&&m_distance[IR_cnt]<36)
								{
									result_measure=m_distance[IR_cnt];
								}

								SENSOR_usart.receivebuf[4] = result_measure/100000+0x30;
								SENSOR_usart.receivebuf[5] = result_measure%100000/10000+0x30;
								SENSOR_usart.receivebuf[6] = result_measure%10000/1000+0x30;
								SENSOR_usart.receivebuf[8] = result_measure%1000/100+0x30;
								SENSOR_usart.receivebuf[9] = result_measure%100/10+0x30;
								SENSOR_usart.receivebuf[10] = result_measure%10+0x30;
						}
						else
						{
							char *p="ERR--18";
							strncpy((char *)(&SENSOR_usart.receivebuf[4]),p,7);							
						}
					}
					else
					{
								if(result_measure>15&&result_measure<50)
								{
									result_measure-=10;
								}
								
								if(result_measure<40&&m_distance[IR_cnt]>0&&m_distance[IR_cnt]<36)
								{
									result_measure=m_distance[IR_cnt];
								}
								
								SENSOR_usart.receivebuf[4] = result_measure/100000+0x30;
								SENSOR_usart.receivebuf[5] = result_measure%100000/10000+0x30;
								SENSOR_usart.receivebuf[6] = result_measure%10000/1000+0x30;
								SENSOR_usart.receivebuf[8] = result_measure%1000/100+0x30;
								SENSOR_usart.receivebuf[9] = result_measure%100/10+0x30;
								SENSOR_usart.receivebuf[10] = result_measure%10+0x30;						
					}
				}
				else if(SENSOR_usart.receivebuf[7] == '-')//新传感器数据返回格式与原传感器返回格式不同，数据移位  有错误情况下没符号，因此向后移位1位，跟之前保持一致
				{
					if(m_distance[IR_cnt]<36)
					{
						result_measure=m_distance[IR_cnt];
						SENSOR_usart.receivebuf[4] = result_measure/100000+0x30;
						SENSOR_usart.receivebuf[5] = result_measure%100000/10000+0x30;
						SENSOR_usart.receivebuf[6] = result_measure%10000/1000+0x30;
						SENSOR_usart.receivebuf[7] = '.';
						SENSOR_usart.receivebuf[8] = result_measure%1000/100+0x30;
						SENSOR_usart.receivebuf[9] = result_measure%100/10+0x30;
						SENSOR_usart.receivebuf[10] = result_measure%10+0x30;
					}
					else
					{		
						switch(SENSOR_usart.receivebuf[9])
						{
							case '5':
								switch(SENSOR_usart.receivebuf[10])
								{
									case '0'://150
										SENSOR_usart.receivebuf[9] = '3';
										SENSOR_usart.receivebuf[10] = '1';
										break;
									case '1'://151
										break;
								}
								break;
							case '6':
								switch(SENSOR_usart.receivebuf[10])
								{
									case '0':
										break;
									case '1':
										SENSOR_usart.receivebuf[9] = '3';
										SENSOR_usart.receivebuf[10] = '2';
										break;
								}	
								break;
							case '7':
								switch(SENSOR_usart.receivebuf[10])
								{
									case '0':
										SENSOR_usart.receivebuf[9] = '3';
										SENSOR_usart.receivebuf[10] = '3';
										break;
									case '1':
										SENSOR_usart.receivebuf[9] = '3';
										SENSOR_usart.receivebuf[10] = '4';
										break;
								}
								break;
							case '8':
								switch(SENSOR_usart.receivebuf[10])
								{
									case '0':
										SENSOR_usart.receivebuf[9] = '3';
										SENSOR_usart.receivebuf[10] = '5';
										break;
									case '1':
										SENSOR_usart.receivebuf[9] = '3';
										SENSOR_usart.receivebuf[10] = '6';
										break;
									case '2':
										SENSOR_usart.receivebuf[9] = '3';
										SENSOR_usart.receivebuf[10] = '7';
										break;
									case '3':
										SENSOR_usart.receivebuf[9] = '3';
										SENSOR_usart.receivebuf[10] = '8';
										break;
									case '4':
										SENSOR_usart.receivebuf[9] = '3';
										SENSOR_usart.receivebuf[10] = '9';
										break;
								}	
								break;
							case '9':
								switch(SENSOR_usart.receivebuf[10])
								{
									case '0':
										SENSOR_usart.receivebuf[9] = '4';
										SENSOR_usart.receivebuf[10] = '1';
										break;
									case '1':
										SENSOR_usart.receivebuf[9] = '4';
										SENSOR_usart.receivebuf[10] = '2';
										break;
								}	
								break;
							case '0':
								break;
							default:
								break;
						}
						for(uint8_t j=11;j>3;j--)
						{
							SENSOR_usart.receivebuf[j]=SENSOR_usart.receivebuf[j-1];
						}
						
					}
				}
				else if(SENSOR_usart.receivebuf[7]!='-')
				{
						char *p="000-000";
						strncpy((char*)(&SENSOR_usart.receivebuf[4]),p,8);					
				}
				
				
				for(uint8_t j = 0; j < 7; j++)
				{
					result_buf[3+j] = SENSOR_usart.receivebuf[j+4];
				}
				result_buf[10] = 0x1D;
				result_buf[11] = 0x0D;
				Blewrite(result_buf,12);
				OFF1_PWEN;
				OFF2_PWEN;
				OFF3_PWEN;
				OFF4_PWEN;				
				end_handle();
		}
		break;
		case 0x14:
		{
				uint32_t result_measure = 0;				
				uint8_t  total_result_buf[180] = {0};				
				uint16_t num = 0xffff;            //每个传感器是否测量标志位
				uint8_t  test_flag[4]={0,0,0,0};//每一组传感器是否测量标志
				uint16_t  timeout = 0;
				uint8_t  cmd_all_measure[4] = {0xfa,0x06,0x06,0xfa};
				uint8_t  num_of_group = 0;
				uint8_t  index_in_group = 0;
				uint8_t  cmd_read[4] = {0x80,0x06,0x07,0x73};
				uint8_t  start_index_sensor=0;
				uint8_t  IR_cnt=0;
				if(BLE_usart.receivebuf[18]!=0x1d||BLE_usart.receivebuf[19]!=0x0d)
				{
					error_handle();
					for(uint8_t i=0;i<SIZE_LEN;i++)
					{
						BLE_usart.receivebuf[i] =  0;
					}
					break;
				}
				start_handle();
				
				num = BLE_usart.receivebuf[1]*256+BLE_usart.receivebuf[2];
				for(uint8_t i=0;i<18;i++)
				{
					total_result_buf[i] = BLE_usart.receivebuf[i];
				}
				test_flag[0] = 0;
				for(uint8_t i=0;i<4;i++)
				{
					if((num>>i)&0x0001)
					{
						test_flag[0] = 1;
						power_on(0);
						Delay(600);
						break;
					}
				}
				if(test_flag[0]==1)
				{
					usart_send_1_4_ctrl(0);
					Delay(10);
					Sensorwrite(cmd_all_measure,4);
					Delay(MEASURE_TIMEOUT);
				}
				
				for(num_of_group=0;num_of_group<4;num_of_group++)
				{
					if(num_of_group<3)
					{
						test_flag[num_of_group+1] = 0;
						for(index_in_group=0;index_in_group<4;index_in_group++)
						{
							if((num>>((num_of_group*4+4)+index_in_group))&0x0001)
							{
								test_flag[num_of_group+1] = 1;
								power_on(num_of_group+1);
								Delay(600);
								break;
							}
						}
						if(test_flag[num_of_group+1] == 1)
						{
							usart_send_1_4_ctrl(num_of_group+1);
							Delay(50);
							Sensorwrite(cmd_all_measure,4);
							Delay(MEASURE_TIMEOUT);
						}
					}
					for(index_in_group = 0;index_in_group < 4;index_in_group++)
					{
						WDT_RESET_COUNTER();
						if((num>>(4*num_of_group+index_in_group))&0x0001)
						{
//							temp_d_buf[0]=0x80+4*num_of_group+i;
//							temp_d_buf[3]=0x73-(4*num_of_group+i);
							usart_receive_1_16_ctrl((index_in_group+num_of_group*4));
							
							SENSOR_usart.reset();                   //清buf计数
							Delay(10);
							Sensorwrite(cmd_read,4);
							Delay(10);
							//读取数据，超时时间为
							timeout = 0;
							while(timeout < 1000)
							{
								SENSOR_usart_service();
								if(SENSOR_usart.flag_complete==1)
								{
									SENSOR_usart.flag_complete=0;
									if(SENSOR_usart.receive_cnt == 0x0c)
									{
										ON_SPK;
										Delay(30);
										OFF_SPK;
										break;
									}
									else if(SENSOR_usart.receive_cnt == 0x0d)//遇到过最前面多一个0x00
									{
											if(SENSOR_usart.receivebuf[2] == 0x06&&SENSOR_usart.receivebuf[3]==0x82)
											{
												for(uint8_t i=0;i<7;i++)
												{
													SENSOR_usart.receivebuf[4+i]=SENSOR_usart.receivebuf[5+i];
												}
											}
											break;
									}
								}
								
//								if((timeout != 0) && (timeout%10 == 0))
//								{
//									SENSOR_usart.receive_cnt = 0;                   //清buf计数
//									Sensorwrite(cmd_read,4);
//									Delay(20);
//								}
								timeout++;
								Delay(1);
							}
							
							SENSOR_usart.receive_cnt = 0;
						}
						else
						{
								char *p="000-000";
								strncpy((char*)&SENSOR_usart.receivebuf[4],p,7);
						}
					//	start_buf[0] = 1+index_in_group+num_of_group*4;
					//	hex_to_char(start_buf,end_buf,1);
						start_index_sensor = 18+(index_in_group + num_of_group*4)*10;
						total_result_buf[start_index_sensor] = 0x24;
						total_result_buf[start_index_sensor+1] = (index_in_group + num_of_group*4+1)/10+0x30;  //传感器序号，转换成字符
						total_result_buf[start_index_sensor+2] = (index_in_group + num_of_group*4+1)%10+0x30;
						
						uint32_t m_distance[5]={0,300,300,300,300};
//						if(4*num_of_group+index_in_group+1>=1&&4*num_of_group+index_in_group+1<=4)
//						{
//							ADC_convert(m_distance);
//							
//							if(m_distance[4*num_of_group+index_in_group+1]>35)
//							{
//								m_distance[4*num_of_group+index_in_group+1]=300;
//							}
//						}
						
						
						if(4*num_of_group+index_in_group+1==2||4*num_of_group+index_in_group+1==6||4*num_of_group+index_in_group+1==9||4*num_of_group+index_in_group+1==13)
						{
							if(4*num_of_group+index_in_group+1==2)
							{
								IR_cnt=1;
							}
							else if(4*num_of_group+index_in_group+1==6)
							{
								IR_cnt=2;
							}
							else if(4*num_of_group+index_in_group+1==9)
							{
								IR_cnt=3;
							}
							else if(4*num_of_group+index_in_group+1==13)
							{
								IR_cnt=4;
							}
							ADC_convert(m_distance);
							
							if(m_distance[IR_cnt]>35)
							{
								m_distance[IR_cnt]=300;
							}
						}
						
						if(SENSOR_usart.receivebuf[7] == 0x2E) //判断是否测量到数据，如果测量到，则中间位数据为：0x2E，即.    000.123米
						{
							for(uint8_t j=4;j<11;j++)
							{
								if((j!=7)&&(SENSOR_usart.receivebuf[j]<0x30||SENSOR_usart.receivebuf[j]>0x39))//如果有一个值不在1-9之间，那么测量到的数据有误；错误码0x19
								{
									char *p="ERR--19";
									strncpy((char*)&SENSOR_usart.receivebuf[4],p,7);
									break;
								}
							}
							if(SENSOR_usart.receivebuf[7]==0x2E)//判断测量数据是否有误，如果没有继续计算补偿值
							{
								
								result_measure = 	 (SENSOR_usart.receivebuf[4]-0x30)*100000+(SENSOR_usart.receivebuf[5]-0x30)*10000
																	+(SENSOR_usart.receivebuf[6]-0x30)*1000+(SENSOR_usart.receivebuf[8]-0x30)*100
																	+(SENSOR_usart.receivebuf[9]-0x30)*10+SENSOR_usart.receivebuf[10]-0x30;
								//printf("result_measure value:[%d] ---\r\n",result_measure);
								if(ERR_Com[2*(index_in_group+4*num_of_group)] == '+')
								{
									result_measure += ERR_Com[2*(index_in_group+4*num_of_group)+1];
									if(result_measure>15&&result_measure<50)
									{
										result_measure-=10;
									}
									if(result_measure<30&&m_distance[IR_cnt]<20)
									{
										//printf("result_measure value:[%d] ---\r\n",result_measure);
										result_measure=m_distance[IR_cnt];
										//printf("result_measure value:[%d] ---\r\n",result_measure);
									}
									
									SENSOR_usart.receivebuf[4] = result_measure/100000+0x30;
									SENSOR_usart.receivebuf[5] = result_measure%100000/10000+0x30;
									SENSOR_usart.receivebuf[6] = result_measure%10000/1000+0x30;
									SENSOR_usart.receivebuf[8] = result_measure%1000/100+0x30;
									SENSOR_usart.receivebuf[9] = result_measure%100/10+0x30;
									SENSOR_usart.receivebuf[10] = result_measure%10+0x30;
								}
								else if(ERR_Com[2*(index_in_group+4*num_of_group)] == '-')
								{
									if(result_measure >= ERR_Com[2*(index_in_group+4*num_of_group)+1])
									{
										result_measure -= ERR_Com[2*(index_in_group+4*num_of_group)+1];
										if(result_measure>15&&result_measure<50)
										{
											result_measure-=10;
										}
										if(result_measure<30&&m_distance[IR_cnt]<30)
										{
//											printf("result_measure value:[%d] ---\r\n",result_measure);
											result_measure=m_distance[IR_cnt];
//											printf("result_measure value:[%d] ---\r\n",result_measure);
//											printf("IR_count value:[%d] ---\r\n",IR_cnt);
//											printf("m_distance1 value:[%d] ---\r\n",m_distance[1]);
//											printf("m_distance2 value:[%d] ---\r\n",m_distance[2]);
//											printf("m_distance3 value:[%d] ---\r\n",m_distance[3]);
//											printf("m_distance4 value:[%d] ---\r\n",m_distance[4]);
										}
										
										SENSOR_usart.receivebuf[4] = result_measure/100000+0x30;
										SENSOR_usart.receivebuf[5] = result_measure%100000/10000+0x30;
										SENSOR_usart.receivebuf[6] = result_measure%10000/1000+0x30;
										SENSOR_usart.receivebuf[8] = result_measure%1000/100+0x30;
										SENSOR_usart.receivebuf[9] = result_measure%100/10+0x30;
										SENSOR_usart.receivebuf[10] = result_measure%10+0x30;

									}
									else
									{
										char *p="ERR--18";
										strncpy((char*)&SENSOR_usart.receivebuf[4],p,7);
									}
								}
								else
								{
										if(result_measure>15&&result_measure<50)
										{
											result_measure-=10;
										}
										if(result_measure<30&&m_distance[IR_cnt]<30)
										{
											result_measure=m_distance[IR_cnt];
										}
										
										SENSOR_usart.receivebuf[4] = result_measure/100000+0x30;
										SENSOR_usart.receivebuf[5] = result_measure%100000/10000+0x30;
										SENSOR_usart.receivebuf[6] = result_measure%10000/1000+0x30;
										SENSOR_usart.receivebuf[8] = result_measure%1000/100+0x30;
										SENSOR_usart.receivebuf[9] = result_measure%100/10+0x30;
										SENSOR_usart.receivebuf[10] = result_measure%10+0x30;
								}
							}
						}
						else if(SENSOR_usart.receivebuf[6] == '-')//如果非正常数据，判断是否为错误码
						{//if(SENSOR_usart.receivebuf[8]=='1'&&SENSOR_usart.receivebuf[9]=='5'&&SENSOR_usart.receivebuf[10]=='1'&&m_distance[IR_cnt]<36)
								if(m_distance[IR_cnt]<36&&m_distance[IR_cnt]>0)
								{
									result_measure=m_distance[IR_cnt];
									SENSOR_usart.receivebuf[4] = result_measure/100000+0x30;
									SENSOR_usart.receivebuf[5] = result_measure%100000/10000+0x30;
									SENSOR_usart.receivebuf[6] = result_measure%10000/1000+0x30;
									SENSOR_usart.receivebuf[7] = '.';
									SENSOR_usart.receivebuf[8] = result_measure%1000/100+0x30;
									SENSOR_usart.receivebuf[9] = result_measure%100/10+0x30;
									SENSOR_usart.receivebuf[10] = result_measure%10+0x30;
								}
								else
								{
									for(uint8_t j=11;j>3;j--)
									{
										SENSOR_usart.receivebuf[j]=SENSOR_usart.receivebuf[j-1];
									}
								}
						}
						else if(SENSOR_usart.receivebuf[7] != '-')//如果测量到的值既不是错误码，也不是正确返回值；则认为没有测量到数据；返回错误码ERR17
						{
							char *p="ERR--17";
							strncpy((char*)&SENSOR_usart.receivebuf[4],p,7);
						}
						
						for(uint8_t j=0;j<7;j++)//将数据复制到result中
						{
							total_result_buf[start_index_sensor+3+j] = SENSOR_usart.receivebuf[4+j];
						}
						for(uint8_t j = 0;j< SIZE_LEN;j++)//复位串口缓存区数据
						{
							SENSOR_usart.receivebuf[j] =  0;
						}
					}
					power_off(num_of_group);
				}
				total_result_buf[178]=0x1D;
				total_result_buf[179]=0x0D;
				Blewrite(total_result_buf,18);
				Delay(20);
				for(uint8_t i = 0;i < 16;i++)
				{
					Blewrite(total_result_buf+18+i*10,10);
					Delay(20);
				}
				Blewrite(total_result_buf+178,2);
				OFF1_PWEN;
				OFF2_PWEN;
				OFF3_PWEN;
				OFF4_PWEN;
//				if(write_format_control(total_result_buf))
//				{
//					error_handle();
//				}
				
				for(uint8_t i=0;i<180;i++)
				{
					total_result_buf[i] = 0;
				}
				for(uint8_t i=0;i<SIZE_LEN;i++)
				{
					SENSOR_usart.receivebuf[i] = 0;
					BLE_usart.receivebuf[i] = 0;
				}
				end_handle();
		}
		break;
		case 0x16:   //擦除Flash中除最后两个扇区外的所有数据
		{
			uint8_t temp[38]={0},rtn[2]={0};
			if(BLE_usart.receivebuf[1]!=0x1d||BLE_usart.receivebuf[2]!=0x0d)
			{
				error_handle();
				for(uint8_t i=0;i<SIZE_LEN;i++)
				{
					BLE_usart.receivebuf[i] =  0;
				}
				break;
			}
			start_handle();
			SPI_Flash_ReadBuffer(temp,0x7FE000,38);
			Delay(100);
//			SPI_Flash_EraseChip(W25X_CMD_ChipErase);
//			SPI_Flash_Writebuffer(temp,0x7FE000,38);
			rtn[0] = 0x16;
			rtn[1] = 0x24;
			Blewrite(rtn,2);
			end_handle();
		}
		break;
		case 0x17://读取电量
		{
			uint8_t i=0,finish_value[7]={0};
			if(BLE_usart.receivebuf[1]!=0x1d||BLE_usart.receivebuf[2]!=0x0d)
			{
				error_handle();
				for(i=0;i<SIZE_LEN;i++)
				{
					BLE_usart.receivebuf[i] =  0;
				}
				break;
			}
			start_handle();
			Delay(1000);
			finish_value[0] =  0x17;
			finish_value[1] =  p_battery[0]/1000+0x30;
			finish_value[2] =  0x2e;
			finish_value[3] =	(p_battery[0]%1000)/100+0x30;
			finish_value[4] = (p_battery[0]%100)/10 + 0x30;
			finish_value[5] =  0x56;
			finish_value[6] = 0x24;
			Blewrite(finish_value,7);
			end_handle();
		}
		break;
		case 0x18:
		{
			
		}
		break;
		case 0x19:
		{
			
		}
		break;
		case 0x30:  //读取自检结果
		{
			uint8_t i = 0;
			uint8_t str_test_result[80]={0};
			uint8_t test_counter = 0;
			if(BLE_usart.receivebuf[1]!=0x1d||BLE_usart.receivebuf[2]!=0x0d)
			{
				for(i=0;i<SIZE_LEN;i++)
				{
					BLE_usart.receivebuf[i] =  0;
				}
				error_handle();
				break;
			 }
			 start_handle();
			 Delay(1000);
			 if(Self_test_flag&1)
			 {
				 str_test_result[test_counter]='/';
				 test_counter += 1;
				 str_test_result[test_counter]='P';
				 test_counter += 1;
				 str_test_result[test_counter]='O';
				 test_counter += 1;
				 str_test_result[test_counter]='W';
				 test_counter += 1;
			 }
			 else if(Self_test_flag&2)
			 {
				 str_test_result[test_counter]='/';
				 test_counter+=1;
				 str_test_result[test_counter]='R';
				 test_counter+=1;
				 str_test_result[test_counter]='T';
				 test_counter+=1;
				 str_test_result[test_counter]='C';
				 test_counter+=1;
			 }
			 else if(Self_test_flag&4)
			 {
				 str_test_result[test_counter]='/';
				 test_counter+=1;
				 str_test_result[test_counter]='F';
				 test_counter+=1;
				 str_test_result[test_counter]='L';
				 test_counter+=1;
				 str_test_result[test_counter]='A';
				 test_counter+=1;
			 }
			 else if(Self_test_flag&8)
			 {
				 str_test_result[test_counter]='/';
				 test_counter+=1;
				 str_test_result[test_counter]='B';
				 test_counter+=1;
				 str_test_result[test_counter]='L';
				 test_counter+=1;
				 str_test_result[test_counter]='T';
				 test_counter+=1;
			 }
			 for(i=0;i<16;i++)
			 {
				 if(Self_sensor_flag&(1<<i))
				 {
					 str_test_result[test_counter] = '/';
					 test_counter += 1;
					 str_test_result[test_counter] = 'S';
					 test_counter += 1;
					 str_test_result[test_counter] = (i+1)/10+0x30;
					 test_counter += 1;
					 str_test_result[test_counter] = (i+1)%10+0x30;
					 test_counter += 1;
				 }
			 }
			 if(test_counter > 1)
			 {
				 Blewrite(str_test_result,test_counter);
			 }
			 else
			 {
					str_test_result[0] = 0x30;
					str_test_result[1] = 0x24;
					Blewrite(str_test_result,2);
			 }
			 for(i = 0;i<SIZE_LEN;i++)
			 {
				 BLE_usart.receivebuf[i] = 0;
			 }			 
			 end_handle();
		}
		break;
		case 0x31:  //设置ID
		{
			 uint8_t i = 0;
			 uint8_t rtn[2]={0};
			 if(BLE_usart.receivebuf[9] != 0x1d||BLE_usart.receivebuf[10] != 0x0d)
			 {
					for(i = 0;i<SIZE_LEN;i++)
					{
						BLE_usart.receivebuf[i] =  0;
					}
					error_handle();
					break;
			 }
			 start_handle();
			 Delay(1000);
			 set_ID((uint8_t*)&BLE_usart.receivebuf[1]);
			 rtn[0] = 0x31;
			 rtn[1] = 0x24;
			 Blewrite(rtn,2);
			 Delay(500);
			 BLE_usart.reset();
			 set_bt_name();
			 end_handle();
		}
		break;
		case 0x32:  //读取ID
		{
			 uint8_t i=0;
			 uint8_t rtn[8]={0};
			 if(BLE_usart.receivebuf[1] != 0x1d||BLE_usart.receivebuf[2] != 0x0d)
			 {
					for(i=0;i<SIZE_LEN;i++)
					{
						BLE_usart.receivebuf[i] =  0;
					}
					error_handle();
					break;
			 }
			 start_handle();
			 Delay(1000);
			 rtn[0] = 0x32;
			 SPI_Flash_ReadBuffer(&rtn[1],0x7FE000,6);
			 Delay(100);
			 rtn[7] = 0x24;
			 Blewrite(rtn,8);
			 for(i=0;i<SIZE_LEN;i++)
			 {
				 BLE_usart.receivebuf[i] = 0;
			 }
			 end_handle();
		}
		break;
		case 0x41:  //误差补偿
		{
			uint8_t i = 0,rtn[2]={0};
			if(BLE_usart.receivebuf[4] != 0x1d||BLE_usart.receivebuf[5] != 0x0d)
			{
				for(i = 0;i < SIZE_LEN;i++)
				{
						BLE_usart.receivebuf[i] =  0;
				}
				error_handle();
				break;
			}
			 start_handle();
			 Delay(1000);
			 set_err((uint8_t*)&BLE_usart.receivebuf[1]);
			 SPI_Flash_ReadBuffer(ERR_Com,0x7FE006,32);
			 for(i = 0;i<SIZE_LEN;i++)
			 {
				 BLE_usart.receivebuf[i] = 0;
			 }
			 rtn[0] = 0x41;
			 rtn[1] = 0x24;
			 Blewrite(rtn,2);
			 end_handle();
		}
		break;
		case 0x50:   //打开激光
		{
			uint8_t cmd_open[5] = {0x80,0x06,0x05,0x01,0x74};
			uint8_t i_open = 0;
			uint8_t i_num_a = 0;
			if(BLE_usart.receivebuf[1] != 0x1D||BLE_usart.receivebuf[2] != 0x0D)
			{
				error_handle();
				break;
			}
			start_handle();
			power_on(0);
			power_on(1);
			power_on(2);
			power_on(3);
			Delay(200);
			for(i_open = 0;i_open <16;i_open++)
			{
				usart_receive_1_16_ctrl(i_open);
				Delay(100);
				cmd_open[0]=0x80;
				cmd_open[4]=0x74;
				SENSOR_usart.receive_cnt = 0;
				Sensorwrite(cmd_open,5);

				SENSOR_usart.receive_cnt = 0;
			}
			for(uint8_t i = 0;i<SIZE_LEN;i++)
			{
				SENSOR_usart.receivebuf[i] = 0;
				BLE_usart.receivebuf[i_num_a] = 0;
			}
			end_handle();
		}
		break;
		case 0x51:   //关闭激光
		{
			if(BLE_usart.receivebuf[1] != 0x1D||BLE_usart.receivebuf[2] != 0x0D)
			{
				error_handle();
				break;
			}
			start_handle();
			power_off(0);
			power_off(1);
			power_off(2);
			power_off(3);
			Delay(500);
			end_handle();
		}
		break;
		
		case 0x52:   //打开激光指示灯
		{
			if(BLE_usart.receivebuf[1] != 0x1D||BLE_usart.receivebuf[2] != 0x0D)
			{
				error_handle();
				break;
			}
			start_handle();
			ON_INDI_POW;
			Delay(500);
			end_handle();
		}
		break;
		case 0x53:   //关闭激光指示灯
		{
			if(BLE_usart.receivebuf[1] != 0x1D||BLE_usart.receivebuf[2] != 0x0D)
			{
				error_handle();
				break;
			}
			start_handle();
			OFF_INDI_POW;
			Delay(500);
			end_handle();
		}
		break;
		case 0x60:
		{
			uint8_t set_ans_time_code[5]={0xFA,0x04,0x0F};
			uint8_t i=0,j=0,delay_temp=0,time_out=0;
			if(BLE_usart.receivebuf[2]!=0x1d||BLE_usart.receivebuf[3]!=0x0d||BLE_usart.receivebuf[1]>0x10)
			{
				error_handle();
				for(i=0;i<SIZE_LEN;i++)
				{
					BLE_usart.receivebuf[i] =  0;
				}
				break;
			}
			start_handle();
			
			delay_temp = BLE_usart.receivebuf[1];
			
//			W25QXX_Erase_Sector(1);
			
			SPI_Flash_Writebuffer(&delay_temp,0x1000,1);
			
			MEASURE_TIMEOUT = (BLE_usart.receivebuf[1]*500+200);
			
			set_ans_time_code[3] =	BLE_usart.receivebuf[1];
			set_ans_time_code[4] =  0xf3-BLE_usart.receivebuf[1];
			power_on(0);
			power_on(1);
			power_on(2);
			power_on(3);
			for(i=0;i<0x10;i++)
			{
				WDT_RESET_COUNTER();
				usart_receive_1_16_ctrl(i);
				
				Delay(100);
				SENSOR_usart.reset();
				Sensorwrite(set_ans_time_code,5);
				Delay(200);
				time_out=0;
				while(time_out < 100)
				{
					SENSOR_usart_service();
					if(SENSOR_usart.flag_complete==1)
					{
						SENSOR_usart.flag_complete=0;
						if(SENSOR_usart.receive_cnt == 0x04)
						{
							uint8_t rtn[2] = {0};
							ON_SPK;
							Delay(10);
							OFF_SPK;
							rtn[0] = i;
							rtn[1] = 0x00;
							Blewrite(rtn,2);
							
						}
						else if(SENSOR_usart.receive_cnt == 0x05)
						{
							uint8_t rtn[2] = {0};
							rtn[0] = i;
							rtn[1] = 0xf0;
							Blewrite(rtn,2);
						}
						break;
					}
					time_out++;
					Delay(30);
				}
				

				if(time_out>=100)
				{
							uint8_t rtn[2] = {0};
							rtn[0] = i;
							rtn[1] = 0xf1;
							Blewrite(rtn,2);
				}						
				for(j = 0;j<SIZE_LEN;j++)
				{
					SENSOR_usart.receivebuf[j] = 0;
				}
				SENSOR_usart.receive_cnt = 0;
			}
			uint8_t rtn[2] = {0};
			rtn[0] = 0x60;
			rtn[1] = 0x24;
			Blewrite(rtn,2);
			for(j = 0;j<SIZE_LEN;j++)
			{
				SENSOR_usart.receivebuf[j] = 0;
				BLE_usart.receivebuf[j] = 0;
			}
			power_off(0);
			power_off(1);
			power_off(2);
			power_off(3);
			Delay(500);
			end_handle();
		}
		break;
	}
	//清除buf
	for(uint8_t i=0;i<SIZE_LEN;i++)
	{
		SENSOR_usart.receivebuf[i]=0x00;
		BLE_usart.receivebuf[i]=0x00;
	}
	//NVIC_DisableIRQ(TIM2_IRQn);
	
}

void set_bt_name()
{
	//查询蓝牙名称
		uint8_t name_temp[6] = {0},device_width[2] = {0},f_name = 0;
		uint8_t str_read[9]="AT+NAME\r\n";
		uint8_t str_set[27]="AT+NAMEBT";
		uint8_t strtest[4]="AT\r\n";
//		BT_RST_Pin = 0;//蓝牙复位
//		Delay(100);
//		BT_RST_Pin = 1;//蓝牙复位
//		Delay(1000);
//		//唤醒蓝牙模块
//		while(f_name<5)
//		{
//			Blewrite(strtest,4);
//			//Delay(5);
//			f_name++;
//		}
//		f_name = 0;
//		while(f_name<10)
//		{
//			BLE_usart_service();//判断蓝牙是否接收完成
//			if(flag_BLE_OK==1)
//			{
//				f_name = 0;
//				flag_BLE_OK = 0;				
//				//Delay(100);
//				break;
//			}
//			Delay(10);
//			f_name++;
//		}
		BT_RST_Pin = 0;
		Delay(100);
		BT_RST_Pin = 1;
		Delay(500);
		BLE_usart.reset();
		BT_EN_Pin = 0;//唤醒蓝牙
		Blewrite(strtest,4);
		Delay(500);
		while(f_name < 15)
		{
			//BLE_usart_service();
			if(flag_BLE_OK==1)
			{
				flag_BLE_OK=0;				
				break;
			}
//			Blewrite(strtest,4);
			Delay(200);
			f_name++;
		}
		BT_EN_Pin = 1;//蓝牙复位,拉低，使蓝牙处于不可连接状态
		
		if(f_name<=15)
		{
			SPI_Flash_ReadBuffer(name_temp,0x7FE000,6);
			Delay(100);
			SPI_Flash_ReadBuffer(device_width,0x7FE026,2);
			Delay(100);
			if(name_temp[0] != 0xFF&&name_temp[1] != 0xFF&&name_temp[2] != 0xFF
			 &&name_temp[3] != 0xFF&&name_temp[4] != 0xFF&&name_temp[5] != 0xFF)
			{
				uint8_t timeout = 0;
				Delay(500);
				BLE_usart.reset();
				BT_EN_Pin = 0;//唤醒蓝牙
				Blewrite(str_read,9);
				Delay(500);
				while(flag_BLE_newline == 0)
				{
					//BLE_usart_service();
					Delay(200);
					timeout++;
					if(timeout>10)
					{
						break;
					}
				}
				Delay(1000);
				BT_EN_Pin = 1;//蓝牙复位,拉低，使蓝牙处于不可连接状态
				for(f_name = 0;f_name < 2;f_name++)
				{
					uint8_t temp_high = 0,temp_low = 0;
					temp_high = device_width[f_name]/16;
					temp_low  = device_width[f_name]%16;
					if(temp_high <= 9)
					{
						temp_high += 0x30;
					}
					else if(temp_high>=10&&temp_high<=15)
					{
						temp_high+=0x37;
					}
					if(temp_low <= 9)
					{
						temp_low+=0x30;
					}
					else if(temp_low>=10&&temp_low<=15)
					{
						temp_low+=0x37;
					}
					str_set[9+2*f_name]=temp_high;
					str_set[9+2*f_name+1]=temp_low;
				}
				
				for(f_name = 0;f_name < 6;f_name++)
				{
					uint8_t temp_high = 0,temp_low = 0;
					temp_high = name_temp[f_name]/16;
					temp_low = name_temp[f_name]%16;
					if(temp_high <= 9)
					{
						temp_high += 0x30;
					}
					else if(temp_high>=10&&temp_high<=15)
					{
						temp_high+=0x37;
					}
					if(temp_low <= 9)
					{
						temp_low+=0x30;
					}
					else if(temp_low>=10&&temp_low<=15)
					{
						temp_low+=0x37;
					}
					str_set[13+2*f_name]=temp_high;
					str_set[13+2*f_name+1]=temp_low;
				}
				str_set[25] = '\r';
				str_set[26] = '\n';
				for(f_name = 0;f_name < 16;f_name++)
				{
					if(BLE_usart.receivebuf[f_name+8] != str_set[9+f_name])//è?1???3??úIDo?2?ò???￡??ò??à??à??3?éè??3éBT+IDo?
					{
						timeout = 0;
						flag_BLE_newline = 0;
						Blewrite(str_set,27);
						Delay(100);
						while(flag_BLE_newline == 0)
						{
							Delay(20);
							timeout++;
							if(timeout>10)
							{
								break;
							}
						}
						break;
					}
				}
				
				BT_RST_Pin = 0;//蓝牙复位
				Delay(100);
				BT_RST_Pin = 1;//蓝牙复位
			}
		}
}


void Self_test()
{
	//TIME timer1={0,0,0,0,0,0};
	uint32_t bat[5]={0};
	uint8_t i=0;
	uint8_t strtest[4]="AT\r\n";
	uint16_t FLASH_ID = 0;
//	RTC_TimeTypeDef sTime;
	Self_sensor_flag = 0;
	Self_test_flag = 0;
	
	uint8_t temp=0;//测量延时时间，从flash中读取出计算；

	SPI_Flash_ReadBuffer(&temp,0x1000,1);
	Delay(100);
	if(temp<10&&temp>0)
	{
		MEASURE_TIMEOUT=temp*500+500;
	}
	else
	{
		MEASURE_TIMEOUT=2500;
	}
	SPI_Flash_ReadBuffer(ERR_Com,0x7FE006,32);
	
	SPI_Flash_ReadBuffer(IR_co,0x7FD000,8);
	
	Delay(100);
	ADC_convert(bat);

	//如果ADC采集电压过高或过低报警
	if(bat[0]>4500||bat[0]<3400)
	{
		Self_test_flag+=1;
	}

	if(Self_test_flag<=0)
	{
		//FALSH TEST
		Delay(10);
		FLASH_ID = SPI_Flash_ReadID();
		//0XEF13,表示芯片型号为W25Q80
		//0XEF14,表示芯片型号为W25Q16
		//0XEF15,表示芯片型号为W25Q32
		//0XEF16,表示芯片型号为W25Q64
		if(FLASH_ID!=0XEF16)
		{
			Self_test_flag+=4;
		}
	}
	if(Self_test_flag<=0)
	{
			uint16_t i= 0;
			uint16_t num_a = 0;
			uint8_t read_state[4] = {80,0x06,0x09,71};
			for(i = 0;i < 16;i++)
			{
				if(i==0)
				{
					power_on(0);
					Delay(self_test_power_on_delay);
				}
				else if(i==4)
				{
					power_off(0);
					power_on(1);
					Delay(self_test_power_on_delay);
				}
				else if(i==8)
				{
					power_off(1);
					power_on(2);
					Delay(self_test_power_on_delay);
				}
				else if(i==12)
				{
					power_off(2);
					power_on(3);
					Delay(self_test_power_on_delay);
				}
				usart_receive_1_16_ctrl(i);
				Delay(10);
				read_state[0] = 0x80;
				read_state[3] = 0x71;
				SENSOR_usart.reset(); 
				Sensorwrite(read_state,4);
				Delay(200);
				//如果没有测量结果，继续发送测量命令
				num_a = 0;
				 while(num_a <= 900)
				 {
					 SENSOR_usart_service();
					 if(SENSOR_usart.flag_complete==1)
					 {
						 if(SENSOR_usart.receive_cnt == 5)
						 {
							 SENSOR_usart.receive_cnt = 0;
							 num_a = 0;
							 break;
						 }
					 }
					 else if(num_a!=0&&num_a%300 == 0)
					 {
						 SENSOR_usart.reset(); 
						 Sensorwrite(read_state,4);
						 Delay(100);
					 }
					 num_a++;
					 Delay(10);
				 }
				 //清除
				 SENSOR_usart.receive_cnt = 0;
				 SENSOR_usart.flag_complete = 0;
				 
				 if(num_a>900 || (SENSOR_usart.receivebuf[2]==0x85&&SENSOR_usart.receivebuf[3]!=0))
				 {
					 Self_sensor_flag = Self_sensor_flag + ((uint16_t)1<<i);
					 //break;
				 }
			}
			power_off(3);
			for(i = 0;i < SIZE_LEN; i++)
			{
				SENSOR_usart.receivebuf[i]=0;
			}
	}
	if(Self_test_flag<=0)
	{
		BT_RST_Pin = 0;
		Delay(100);
		BT_RST_Pin = 1;
		Delay(1000);
		BLE_usart.reset();
		BT_EN_Pin = 0;//唤醒蓝牙
		Delay(500);
		Blewrite(strtest,4);
		Delay(300);
		while(i<15)
		{
			//BLE_usart_service();
			if(flag_BLE_OK==1)
			{
				flag_BLE_OK=0;				
				break;
			}
			Blewrite(strtest,4);
			Delay(200);
			i++;
		}
		BT_EN_Pin = 1;//蓝牙复位,拉低，使蓝牙处于不可连接状态
		if(i>=15)
		{
			Self_test_flag+=8;
		}
	}
	SPI_Flash_ReadBuffer(ERR_Com,0x7FE006,32);
	Delay(100);
	if(Self_sensor_flag > 0||Self_test_flag > 0)
	{
		ON_SPK;
	  Delay(1000);
	  OFF_SPK;
	}
	else
	{
		ON_SPK;
		Delay(100);
		OFF_SPK;
		Delay(100);
		ON_SPK;
		Delay(100);
		OFF_SPK;
	}
}

//GPIO_PIN_SET

//GPIO_PIN_RESET

//HAL_GPIO_WritePin


void cheak_CMD(void)
{
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
	}
}

