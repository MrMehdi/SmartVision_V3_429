#include <stdio.h>
#include <string.h>
#include "stm32f4xx.h"
#include "SCCB.h"
#include "camdrv.h"

volatile unsigned d_m;
void Delayms(unsigned iv);
volatile unsigned char JpegBuffer[120*1024];
volatile unsigned char jpegCaptureDoneITFLG;

void SysTick_Handler(void)
{
	if(d_m)d_m--;
}

void DCMI_IRQHandler(void)
{  	   
	if (DCMI_GetITStatus(DCMI_IT_FRAME) == 1) 
	{		
		DCMI_ClearITPendingBit(DCMI_IT_FRAME); 			
		jpegCaptureDoneITFLG = 1;
	}	
}



void SerComConfig(void)
{
	//STMTX-UART7_TX  STMRX-UART7_RX ???
	//CAMTX-USART2_RX CAMRX-USART2_TX
	
  USART_InitTypeDef USART_InitStructure;
	
  GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	//CONFIGURE NORDIC UART
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
	
  /* Enable USART */
  USART_Cmd(USART2, ENABLE);
}

void Cam_DMA_Init(void)
{
  DMA_InitTypeDef  DMA_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
	
	/* Configures the DMA2 to transfer Data from DCMI to the LCD ****************/
  /* Enable DMA2 clock */
	
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);   
  /* DMA2 Stream1 Configuration */  
  DMA_DeInit(DMA2_Stream1);

  DMA_InitStructure.DMA_Channel = DMA_Channel_1;  
  DMA_InitStructure.DMA_PeripheralBaseAddr = DCMI_DR_ADDRESS;	
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)JpegBuffer;	
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = 0xFFFF;  
	//DMA_InitStructure.DMA_BufferSize = 1;  
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  //DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  //DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	//DMA_InitStructure.DMA_Priority =DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;        
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	//DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_INC4;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA2_Stream1, &DMA_InitStructure); 
		
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream1_IRQn;                    
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
}

void Cam_DCMI_Config(void)
{
	/*Vision

HREF	PA4
PCLK	PA6
VSYN	PB7

D0		PC6
D1		PC7
D2 		PC8
D3		PC9
D4		PC11c
D5		PB6
D6		PB8
D7		PB9
*/  
/*Vision*/
  DCMI_InitTypeDef DCMI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  /* Enable DCMI GPIOs clocks */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |  RCC_AHB1Periph_GPIOC |
                         RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE, ENABLE);
  /* Enable DCMI clock */
  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_DCMI, ENABLE);


  GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_DCMI); //HSYNC
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_DCMI); //PCLK
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_DCMI); //VSYNC
	
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_DCMI); //DCMI0
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_DCMI); //DCMI1
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_DCMI); //DCMI2
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_DCMI); //DCMI3
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_DCMI);//DCMI4
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_DCMI); //DCMI5
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_DCMI); //DCMI6
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_DCMI); //DCMI7

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;  

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_11;  //D0 - D4
  GPIO_Init(GPIOC, &GPIO_InitStructure);
	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7| GPIO_Pin_8 | GPIO_Pin_9;  //D5 - D7, VSYNC
  GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;  
  GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;  //HSYNC
  GPIO_Init(GPIOA, &GPIO_InitStructure);	

  // PCLK(PA6)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
		
  /* DCMI configuration *******************************************************/ 
  DCMI_InitStructure.DCMI_CaptureMode = DCMI_CaptureMode_SnapShot;
  DCMI_InitStructure.DCMI_SynchroMode = DCMI_SynchroMode_Embedded;
  DCMI_InitStructure.DCMI_PCKPolarity = DCMI_PCKPolarity_Rising;
  DCMI_InitStructure.DCMI_VSPolarity = DCMI_VSPolarity_Low; //DCMI_VSPolarity_High
  DCMI_InitStructure.DCMI_HSPolarity = DCMI_HSPolarity_Low; //DCMI_HSPolarity_High
  DCMI_InitStructure.DCMI_CaptureRate = DCMI_CaptureRate_All_Frame;//DCMI_CaptureRate_All_Frame
  DCMI_InitStructure.DCMI_ExtendedDataMode = DCMI_ExtendedDataMode_8b; //?
  
  DCMI_Init(&DCMI_InitStructure);

	DCMI_JPEGCmd(ENABLE);
      
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  NVIC_InitStructure.NVIC_IRQChannel = DCMI_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  
	NVIC_Init(&NVIC_InitStructure); 

  DCMI_ITConfig(DCMI_IT_FRAME, ENABLE);

}

int main(void)
{
	static unsigned char r=0,s[2];
	u32 i;
	GPIO_InitTypeDef GPIO_InitStruct;	
	SysTick_Config(SystemCoreClock / 1000); //1Ms Tick,used for delay,updated on core clock freq changes
	Delayms(50);//Power on delay
	
	SCCB_GPIO_Config();
	
	SerComConfig();
	
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_3 | GPIO_Pin_0;
	GPIO_Init(GPIOB, &GPIO_InitStruct);	
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_Init(GPIOA, &GPIO_InitStruct);	
	
	z:
	
	GPIO_SetBits(GPIOB, GPIO_Pin_0);//Enable LDO
	Delayms(1);
	GPIO_SetBits(GPIOB, GPIO_Pin_3);//Disable reset
	Delayms(1);
	GPIO_SetBits(GPIOA, GPIO_Pin_12);//Enable Power down 1
	Delayms(1);
	GPIO_ResetBits(GPIOA, GPIO_Pin_11);//Disable Power down 1
	
	Delayms(100);//Power on delay

	
	Cam_DCMI_Config();
	Cam_DMA_Init();
	Delayms(100);//Power on delay
	

	
	SCCBSetAdress(OV2640_DEVICE_WRITE_ADDRESS);//required by api
	r = CamApi_Init_Regs();//After this pxclock will start
	
	Delayms(500);//Power on delay
	DMA_Cmd(DMA2_Stream1, ENABLE);	
	DCMI_Cmd(ENABLE);
	DCMI_CaptureCmd(ENABLE);
	//Delayms(100);//Power on delay
	
	while(jpegCaptureDoneITFLG == 0)
	{
		
	}
	//Delayms(100);//Power on delay
	
	DMA_Cmd(DMA2_Stream1, DISABLE);	
	DCMI_Cmd(DISABLE);
	DCMI_CaptureCmd(DISABLE);
	
	i = 0;
	USART_SendData(USART2,0xFF);
  while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == 0);
	
	while(1)
	{		
		s[1] = s[0];
		s[0] = JpegBuffer[i++];
		
		if(s[1] == 0xFF && s[0] == 0xD9)
		{
			USART_SendData(USART2,0xD9);
			while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == 0);
			break;
		}
			
		
		if(i > 120*1024)
			break;
		
		USART_SendData(USART2,s[0]);
		while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == 0);
	}
	GPIO_ResetBits(GPIOB, GPIO_Pin_3);//Enable reset
	GPIO_SetBits(GPIOA, GPIO_Pin_12);//Enable Power down 1
	GPIO_SetBits(GPIOA, GPIO_Pin_11);//Enable Power down 1
	Delayms(100);

	
	while(1);
	return 0;
}

void Delayms(unsigned iv)
{
	d_m = iv;
	while(d_m);//reduced every 1 Msec in Systick Interrupt
}
