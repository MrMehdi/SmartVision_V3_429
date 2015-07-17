#include "serialcom.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "camdrv.h"
#include "flash.h"


#define NORDIC_UART_BAUD    115200
#define STM32_DBG_UART_BAUD 115200


#define COM_MAX_SZ 20 // size of the largest command
#define COM_NORDIC_BLOCK_SZ 64


volatile u8  uRxData[USART_RX_BUFFER_SZ];
volatile u16 uRxRp=0,uRxWp=0;
volatile u8  uRxFullFlg = 0;
u8  rxdata[COM_MAX_SZ];//command received

void SerComParseByte(u8* b,u16 sz);


void USART2_IRQHandler(void)//Camera Com
{
	//array is implemented as circular buffer for low latency operations
	if( USART_GetITStatus(USART2, USART_IT_RXNE) ){

		if(uRxFullFlg == 0)
			uRxData[uRxWp++] = USART2->DR;//Store this byte
		else
			USART2->DR;
		
		uRxWp &= (USART_RX_BUFFER_SZ-1);//Wrap around
		
		
		if(uRxWp == uRxRp){
			uRxFullFlg = 1;//Buffer is full				
		}
	}
}
u16 SerComCalculateUsedSpace(void)
{
	u16 sp;
	//Calculate used space
	if(uRxFullFlg == 0){
		if( uRxWp >= uRxRp)
			sp = uRxWp - uRxRp;
		else
			sp = USART_RX_BUFFER_SZ - uRxRp + uRxWp;
	}
	else{
			sp = USART_RX_BUFFER_SZ;
	}
	return sp;
}

void SerComProcess(void)
{
	u16 used;
	u16 i,j,k;
	used = SerComCalculateUsedSpace();
	
	if( used > 4 ){
		for( i = 0;i < used; i++)
		{
			if ( uRxData[ (uRxRp + i +0) & USART_RX_BUFFER_SZ-1] == 0xFF	&&
					 uRxData[ (uRxRp + i +1) & USART_RX_BUFFER_SZ-1] == 0xFE ){//Header
							for(j = i + 2;j<used-1; j++){
								if ( uRxData[ (uRxRp + j +0) & USART_RX_BUFFER_SZ-1] == 0xFF	&&
										 uRxData[ (uRxRp + j +1) & USART_RX_BUFFER_SZ-1] == 0xFA ){//Footer
											 if( j-i-2 < COM_MAX_SZ){
												 for(k=0;k<j-i-2;k++){
													 rxdata[k] = uRxData[ (uRxRp + i + k + 2) & USART_RX_BUFFER_SZ-1];//store message for further processing
												 }
												 uRxRp += (j + 2);//Increment read pointer
												 uRxRp &= USART_RX_BUFFER_SZ-1;
												 SerComParseByte(rxdata,j-i-2);//Parse this message
											 }
											 else
											 {
													uRxRp += (j + 2);//Increment read pointer only too large message
													uRxRp &= USART_RX_BUFFER_SZ-1;
											 }
											 break;
								}															 
							}
				 }											 
		}
	}
	
	if( uRxFullFlg )//No command recieved empty full buffer
		uRxWp = uRxRp = uRxFullFlg = 0;

	
		
	if(uRxRp > USART_RX_BUFFER_SZ-1){
		uRxRp = 0;//Wrap around
	}
	uRxFullFlg = 0;
		
}

void SerComSendRsp(u8 com,u8* d,u32 n)
{
	u8 tmp[COM_MAX_SZ];
	
	tmp[0] = 0xFF;		//Header
	tmp[1] = 0xFE;
	tmp[2] = com;
	memcpy(&tmp[3],d,n); //payload
	tmp[n + 3] = 0xFF;	//Footer
	tmp[n + 4] = 0xFA;
	SerComSendArr((void*)tmp,n+5);//send response
}

void SerComParseByte(u8* b,u16 sz)
{
	u8 sb[30];
	u8 cb[30];
	camera_setting cs;
	u8 JpegBuffer[4];u32 i,k;
	u32 flash_img_sz=0;
	u32 nseq=0;
	
	switch(b[0])
	{
		
		case 0x01: //take picture
			
		  if(sz > 1)goto err_com;
		  
			//Flash_Read(FLASH_CAM_SETTING_ADD,(void*)&cs,sizeof(cs));//read camera settings from internal flash
			
			cb[0] = Cam_click_images(cs.cdly,&nseq,&flash_img_sz);//click images;
			
			SerComSendRsp(1,cb,1);	//report failure of image acquisition to PC
	
			//store image to flash here
			
			SerComSendMessageUser("Total Image's Size ");
			sprintf((void*)sb,"%d Kbytes \r\n",flash_img_sz/1024);
			SerComSendMessageUser(sb);

			
			cb[4] = (unsigned char) (nseq >>24);
			cb[5] = (unsigned char) (nseq >>16);
			cb[6] = (unsigned char) (nseq >> 8);
			cb[7] = (unsigned char) (nseq >> 0);
			
			cb[0] = (unsigned char) (flash_img_sz >>24);
			cb[1] = (unsigned char) (flash_img_sz >>16);
			cb[2] = (unsigned char) (flash_img_sz >> 8);
			cb[3] = (unsigned char) (flash_img_sz >> 0);
			
			SerComSendRsp(2,(void*)cb,8);//send response
			Cam_LED(DISABLE);
			
		break;
			
		case 0x04:
			if(sz != 2)goto err_com;
		
			if(b[1] < FTB_FILES_TOTAL)
			{
				Flash_dump_imagedata(b[1]);
			}
			else
			{
				SerComSendMessageUser("File no doesnt Exist\r\n");
			}
		break;
		case 0x0b: // Display File Table
			 Flash_Dump_filetable();
		break;
		case 0x0c: // Display File Table
			Flash_Create_FS();
			Flash_Dump_filetable();
		break;
		case 0x08:
			SerComSendMessageUser("Loading Default Settings\r\n");
			CamApi_load_default_settings(&cs);
			cs.header = 0x01ABCDEF;
			SerComSendMessageUser("Storing Settings to Flash\r\n");
			cb[0] = Flash_Write(FLASH_CAM_SETTING_ADD,(void*)&cs,sizeof(cs));
			SerComSendRsp(8,cb,1);	
		break;
		
		case 0x09:
//			SerComSendMessageUser("Initializing Camera registers\r\n");
//			cb[0] |= Cam_Init_Registers(&cs);//Initialize both camera registers to prepare for click,both cameras in power down state on return
//			SerComSendRsp(9,cb,1);
		break;
		
		case 0x0a:
			SerComSendMessageUser("Rebooting\r\n");
			cb[0] = 0;
			SerComSendRsp(10,cb,1);
			NVIC_SystemReset();
		break;
		case 0x05://write camera settings
			goto err_com;
			if(sz != 7)goto err_com;
			
		
		break;	
		case 0x06://read camera settings
			if(sz > 1)goto err_com;
      goto err_com;
			
		break;	
		case 0x07: //request Firmware revision
			
		break;
		case 0x03: //request image block //No longer supported
				SerComSendMessageUser("Not Supported\r\n");
				goto err_com;

		break;
		
		default:
			goto err_com;
			break;
	}
return;
	
	err_com:
	SerComSendMessageUser("Invalid Command or ECHO was sent\r\n");
	//cb[0] = 0x0A;//Invalid command response
	//SerComSendRsp(b[0],(void*)cb,1);//send response
	
}

void SerComSendByte(u8 b){
	while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == 0);
	USART_SendData(USART2,b);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == 0);
}

void SerComSendStr(u8* s){
	while(*s != 0)
		SerComSendByte(*s++);
}

void SerComSendArr(u8* b, u32 n){
	u16 i = 0;
	while(n>0){
		SerComSendByte(b[i]);
		n--;
		i++;
	}
}

//void SerComSendMessageUser(const u8 * s){
//#if USER_MSG_ENB == 1
//	while(*s != 0)
//	{
//		while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == 0);
//		USART_SendData(USART2,*s++);
//	}
//#endif
//}
void SerComSendArrayUser(u8* b,u32 n)
{
	while(n)
	{
		while (USART_GetFlagStatus(UART7, USART_FLAG_TC) == 0);
		USART_SendData(UART7,*b);
		while (USART_GetFlagStatus(UART7, USART_FLAG_TC) == 0);
		b++;n--;
	}
}
void SerComSendMessageUser(const u8 * s){
#ifdef USER_MSG_ENB
	while(*s != 0)
	{
		while (USART_GetFlagStatus(UART7, USART_FLAG_TC) == 0);
		USART_SendData(UART7,*s++);
	}
#endif
}

void SerComConfig(void)
{
	
	
	//CAMTXRX is link between NORDIC and STM
	//STMTXRX is link between STM32 and PC
	
	//STMTX - UART7TX - PE8 - PIN39
	//STMRX - UART7RX - PE7 - PIN38
	
	
	USART_InitTypeDef USART_InitStructure;

	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	//CONFIGURE NORDIC-STM UART
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);//A2 A3
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);	
	
		/* Configure USART Tx as alternate function  */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* Connect PXx to USARTx_Tx*/
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
 	/* Connect PXx to USARTx_Rx*/
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);
	
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode =USART_Mode_Rx | USART_Mode_Tx;/* Connect PXx to USARTx_Rx*/
	USART_Init(USART2, &USART_InitStructure);

	//Configure interupt on RX
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); 	
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;		 // we want to configure the USART2 interrupts
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // this sets the priority group of the USART3interrupts
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		  // this sets the subpriority inside the group
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  // the USART1 interrupts are globally enabled
	NVIC_Init(&NVIC_InitStructure);	
	
	/* Enable USART */
	USART_Cmd(USART2, ENABLE);
	
	Delayms(1);
	//CONFIGURE STM-PC UART
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART7, ENABLE);//E7 E8
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);		
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

	GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource8, GPIO_AF_UART7);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource7, GPIO_AF_UART7);
 	
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode =USART_Mode_Rx | USART_Mode_Tx;/* Connect PXx to USARTx_Rx*/
	USART_Init(UART7, &USART_InitStructure);

  
	/* Enable USART */
	USART_Cmd(UART7, ENABLE);
	Delayms(1);
}
