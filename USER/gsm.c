#include "gsm.h"
#include <stdio.h>
#include <string.h>

#include "debug.h"

uint8_t GSM_state_f = OFFLINE;
RingBuffer Gsm_RxBuff;
RingBuffer Gsm_TxBuff;


DECLARE_TASK(GSM_On)
{
	DEBUGFMSG_START(); 
	TaskSuspend(GSM_Ping);
	if(GSM_state_f == OFF)
	{
		PIN_OFF(GSM_ON_OFF);
		DEBUGFMSG("PIN_OFF\r"); 
		T_Delay(3000);
		PIN_ON(GSM_ON_OFF);
		DEBUGFMSG("PIN_ON\r"); 
		T_Delay(500);
	}		
	TaskResume(GSM_Ping);
}

DECLARE_TASK(GSM_Call)
{
	uint8_t AT[] = "ATD+380930893448;\r";
	DEBUGFMSG("GSM_Call\r\n");
//	GSM_MsgSend(AT, sizeof(AT));
	TaskSuspend(GSM_Call);
}


DECLARE_TASK(GSM_Ping)
{
	bool exit = false;
	uint8_t AT[] = {"AT\r"};
	uint8_t msgBuff[50];
	memset(msgBuff, 0, 50);

		DEBUGFMSG("..\r\n"); 
	
		GSM_MsgSend(AT, sizeof(AT));   //ATOMIC BUFFER
		delay_ms(500);								 //ATOMIC BUFFER!!!
		GSM_MsgGet(msgBuff);					 //ATOMIC BUFFER
	
		if(strstr(msgBuff, "OK") != 0) //GSM - online and ON
		{
			GSM_state_f = ON; 
			printf("GSM_OK\r\n"); 
			
			TaskResume(GSM_Actions);
			SetTimerTaskInfin(GSM_Actions, 0, 1000);
			
			ClearTimerTask(GSM_Ping);
			return;
		} 
		else if (strstr(msgBuff, "AT") != 0) //GSM - online but OFF
		{
			GSM_state_f = OFF; 
		} 
		else //GSM - offline
		{
			GSM_state_f = OFFLINE;
		} 
		DEBUGMSG("%s \r\n",msgBuff); 
		memset(msgBuff, 0, 50);	
	
	if(GSM_state_f != OFFLINE) 
	{
			SetTimerTaskInfin(GSM_On, 500, 0);
	}

//	GSM_MsgSend("ATD+380930893448;\r\n"); 
}


DECLARE_TASK(GSM_Actions)
{
	if(GSM_state_f == ON)
	{
		DEBUGFMSG("OK\r\n"); 
	}
	else
	{
		DEBUGFMSG("FAIL\r\n"); 
		TaskSuspend(GSM_Actions);
		SetTimerTaskInfin(GSM_Ping, 0, 1000);
	}
}

void GSM_MsgSend(uint8_t* data_buff, uint8_t sz)
{
	for(int i = 0; i<sz; i++)
	{
		WriteByte(&GSM_TX_BUFF, data_buff[i]);
	}
	
	USART_SendData(GSM_UART, ReadByte(&GSM_TX_BUFF));
	USART_ITConfig(GSM_UART, USART_IT_TC, ENABLE);
}

int GSM_MsgGet(uint8_t* data_buff)
{
	int sz = GetAmount(&GSM_RX_BUFF);
	uint8_t byte = 0;
	uint8_t k = 0;
	for(int i = 0; i<sz; i++)
	{
		byte = ReadByte(&GSM_RX_BUFF);
		if(byte != 0) //Skip zero bytes inside string!
		{
			data_buff[k++] = byte;
		}
	}
	return k;
}

void GSM_IRQHandler(void)
{
	char t = 0;
  if(USART_GetITStatus(GSM_UART, USART_IT_RXNE) != RESET)
  {
			t = USART_ReceiveData(GSM_UART);
			WriteByte(&GSM_RX_BUFF, t);
  }
	if(USART_GetITStatus(GSM_UART, USART_IT_TC) != RESET)
  {
    USART_ClearITPendingBit(GSM_UART, USART_IT_TC);
    if(!IsEmpty(&GSM_TX_BUFF))
		{
			USART_SendData(GSM_UART,ReadByte(&GSM_TX_BUFF));
		}
		else
		{
			USART_ITConfig(GSM_UART, USART_IT_TC, DISABLE);
		}
  }
}