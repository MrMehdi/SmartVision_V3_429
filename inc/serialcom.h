#ifndef __serialcom_h
#define __serialcom_h

	#define USART_RX_BUFFER_SZ 32 //should be power of 2
	
	#include "stm32f4xx.h"
	
	void SerComConfig(void);
	void SerComSendStr(u8* s);
	void SerComSendArr(u8* b, u32 n);
	void SerComSendByte(u8 b);
	void SerComProcess(void);
	void SerComSendRsp(u8 com,u8* d,u32 n);
	void SerComSendMessageUser(const u8 * s);
	void SerComSendArrayUser(u8* b,u32 n);
	void USART2_IRQHandler(void);
	
	#define ERROR_HARD_SRAM		1
	#define ERROR_HARD_FLASH  2
	#define ERROR_HARD_CAM1		3
	#define ERROR_HARD_CAM2		4
	extern u8 sb[50];
#endif
