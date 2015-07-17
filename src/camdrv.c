#include "stm32f4xx_usart.h"
#include "stm32f4xx.h"
#include "misc.h"
#include <stdio.h>
#include "SCCB.h"
#include "OV2640.h"
#include "ov2640_regs.h"
#include "camdrv.h"
#include "serialcom.h"
#include "flash.h"


#define CAM_LED_PORT 	GPIOB
#define CAM_LED_PIN		GPIO_Pin_2

#define CAM_PWD1_PORT	GPIOA
#define CAM_PWD1_PIN	GPIO_Pin_11

#define CAM_PWD2_PORT	GPIOA
#define CAM_PWD2_PIN	GPIO_Pin_12

#define CAM_LDO_PORT	GPIOB
#define CAM_LDO_PIN		0

#define CAM_RST_PORT	GPIOB
#define CAM_RST_PIN		3

#define CAM_RST_ENB_STATE 1
#define CAM_PWD_ENB_STATE 0
#define CAM_LDO_ENB_STATE 1

#define CAMERA_IMGCAPTURE_TIMEOUT 	2000 //milliseconds
#define EXP_TIME_MS 				5
#define MAX_IMAGE_SZ				256*1024

#define ENB_LDO() GPIO_SetBits(GPIOA, GPIO_Pin_5) //Enable LDO
#define DIS_LDO() GPIO_ResetBits(GPIOA, GPIO_Pin_5) //Disable LDO

#define ENB_RST() GPIO_ResetBits(GPIOA, GPIO_Pin_9)//Enable reset
#define DIS_RST() GPIO_SetBits(GPIOA, GPIO_Pin_9) //Disable reset

#define ENB_PWDN_CAM1() GPIO_SetBits(GPIOA, GPIO_Pin_11) //Enable power down
#define DIS_PWDN_CAM1() GPIO_ResetBits(GPIOA, GPIO_Pin_11) //Enable power down

#define ENB_PWDN_CAM2() GPIO_SetBits(GPIOA, GPIO_Pin_12) //Enable power down
#define DIS_PWDN_CAM2() GPIO_ResetBits(GPIOA, GPIO_Pin_12) //Enable power down

#define SLEEP_CAM12() ENB_PWDN_CAM1();ENB_PWDN_CAM2()
#define ENB_CAM1_ACCESS() DIS_PWDN_CAM1();ENB_PWDN_CAM2()
#define ENB_CAM2_ACCESS() DIS_PWDN_CAM2();ENB_PWDN_CAM1()

extern void Delayms(unsigned iv);
extern volatile unsigned char jpegCaptureDoneITFLG;
volatile unsigned jpegCaptureTime;
volatile unsigned jpegTotalTime;

unsigned char JpegBuffer[JPEG_BUFFER_SZ];
volatile unsigned char jpegCaptureDoneITFLG; //Set in interrupt

extern u32 FlashFileBytesWritten;
//Offload DMA buffers for image 2
u8 Jpeg_b1[OFFLOAD_BUFFER_1SZ];
u8 Jpeg_b2[OFFLOAD_BUFFER_2SZ] __attribute__ ((section(".cmm")));

di imgdebug;

camera_setting cs_flash;

u16 click_dly;

void DCMI_IRQHandler(void)
{  	   
	if (DCMI_GetITStatus(DCMI_IT_FRAME) == 1) 
	{		
		DCMI_ClearITPendingBit(DCMI_IT_FRAME); 			
		jpegCaptureDoneITFLG = 1;
	}	
}

void Cam_LED(unsigned char st)
{
	GPIO_WriteBit(CAM_LED_PORT, CAM_LED_PIN,st);
}
void Cam_Reset(unsigned char st)
{
	GPIO_WriteBit(CAM_RST_PORT, CAM_RST_PIN,st);
}

void Cam_Power_Down(unsigned char cam,unsigned char st)
{
	if(cam == CAM1)
	{
		GPIO_WriteBit(CAM_PWD1_PORT, CAM_PWD1_PIN, st);
	}
	if(cam == CAM2)
	{
		GPIO_WriteBit(CAM_PWD2_PORT, CAM_PWD2_PIN, st);
	}
	else
	{
		assert_param(0);//invalid parameter passed
	}
}

void enable_clock(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
			RCC_ClockSecuritySystemCmd(ENABLE);
			/* Enable GPIOs clocks */
			RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
			GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_MCO);
			/* Configure MCO (PA8) */
			GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
			GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
			GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;  //UP
			GPIO_Init(GPIOA, &GPIO_InitStructure);
			RCC_MCO1Config(RCC_MCO1Source_HSI, RCC_MCO1Div_1);// 16MHZ
}

void disable_clock(void)
{
	
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_ClockSecuritySystemCmd(ENABLE);
	/* Enable GPIOs clocks */
	return;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);			
	/* Configure MCO (PA8) */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;  //UP
	GPIO_Init(GPIOA, &GPIO_InitStructure);
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
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_INC4;
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
	
	DCMI_DeInit();
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
  DCMI_InitStructure.DCMI_SynchroMode = DCMI_SynchroMode_Hardware;
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

void Cam_IO_Configure(void)
{

	GPIO_InitTypeDef GPIO_InitStruct;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_5 |  GPIO_Pin_9;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

}



void Cam_Configure_Peripherals(void)
{
	Cam_IO_Configure();
	Cam_DCMI_Config();
	Cam_DMA_Init();
	SLEEP_CAM12();
	DIS_LDO();
}


u8 Cam_Init_Power(void)
{	
	u8 err=0;
	SCCBSetAdress(OV2640_DEVICE_WRITE_ADDRESS);//required by api
	enable_clock();
	Delayms(1);
	ENB_LDO();
	Delayms(10);
	SerComSendMessageUser("Hard-reset Cameras\r\n");
	ENB_RST();
	Delayms(50);
	DIS_RST();           
	Delayms(50);
  SLEEP_CAM12();
	return err;
}

u8 Cam_Get_Image(void)
{

	DCMI_Cmd(ENABLE);
	DMA_Cmd(DMA2_Stream1, ENABLE);
	DCMI_CaptureCmd(ENABLE);
	
	jpegCaptureDoneITFLG = 0;
	jpegCaptureTime	= 0;
	
	while(jpegCaptureDoneITFLG ==0)
	{
		if(jpegCaptureTime > CAMERA_IMGCAPTURE_TIMEOUT)
		{
			jpegTotalTime += CAMERA_IMGCAPTURE_TIMEOUT;//Camera image capture failed
			SerComSendMessageUser("Camera Capture Timed out\r\n");
			return 0;
		}
	}
	jpegTotalTime += jpegCaptureTime;	//store total capture time
	
	return 1;
		
}

u8 Cam_click_images(u16 dly,u32 * nseq,u32 * fsz)
{
	u8 err=0;
	u8 sb[50];
	u8 handle;
	u32 seq;
	u32 sz[2],i,k = 0;
	jpegTotalTime = 0;
	sz[0] = sz[1] = 0;
	
	
	ENB_CAM1_ACCESS();
	CamApi_Init_Regs();

	
	if(Cam_Get_Image()==0)//capture image on DCMI
		err |= ERROR_HARD_CAM1;
	
	ENB_CAM2_ACCESS();
	CamApi_Init_Regs();
	
  //move image to buffer and calculate image size
  if(JpegBuffer[0] == 0xFF && JpegBuffer[1] == 0xD8)
	{
		i=0;
		while(i < JPEG_BUFFER_SZ)
		{		
		
			if(i < OFFLOAD_BUFFER_1SZ)
			{
				Jpeg_b1[i] = JpegBuffer[i];//store in offload buffer 1
			}
			else
			{
				Jpeg_b2[ i - OFFLOAD_BUFFER_1SZ] = JpegBuffer[i];//store in offload buffer 2
			}
			if(i > 2 )
			{
				if(JpegBuffer[i] == 0xD9 && JpegBuffer[i-1] == 0xFF)
				{
					sz[0] = i+1;//store size of this image
					break;
				}	
			}
			i++;
		}
	}
	
	JpegBuffer[0] = 0;//clear jpeg header
	
	DCMI_Cmd(DISABLE);//Prepare for next image capture
	DCMI_CaptureCmd(DISABLE);
	Cam_DCMI_Config();
	Cam_DMA_Init();
	
	//Reset DMA if required
	if(Cam_Get_Image()==0)					//capture image on DCMI
		err |= ERROR_HARD_CAM2;
	 
	
	SLEEP_CAM12();
	
	DMA_Cmd(DMA2_Stream1, DISABLE);	
	DCMI_Cmd(DISABLE);
	DCMI_CaptureCmd(DISABLE);
	Cam_DCMI_Config();
	Cam_DMA_Init();
	
	//Get size of second image
	if(JpegBuffer[0] == 0xFF && JpegBuffer[1] == 0xD8)
	{
		for(i=0;i<JPEG_BUFFER_SZ;i++)
		{
			if(JpegBuffer[i] == 0xFF && JpegBuffer[i+1] == 0xD9)
			{
				sz[1] = i+2;
				break;
			}
		}
	}

	if(sz[0] > 0 || sz [1] > 0 )
	{	
		//Get a File Access		
		handle = FlashCreateNewFile(&seq);
		i = 0;//This is the variabe to count bytes written
		
		//Write debug imformation header , settings poplated on bootup
		imgdebug.header= 0XABCDEF01;
		imgdebug.captime = jpegTotalTime;
		imgdebug.error = err;
		imgdebug.filesz[0]=sz[1];
		imgdebug.filesz[1]=sz[0];
		imgdebug.seqno=seq;
		imgdebug.footer = 0xFFFE;
		
	  FlashFileWrite(i,handle,(void*)&imgdebug.header,sizeof(imgdebug));
		i = 512;//Leaving a 512 byte space to reduce RAM . TODO optimize this
		

		if(sz[0] > 0 )//write Second file
		{
			i += FlashFileWrite(i,handle,JpegBuffer,sz[1]);
#ifdef UART_IMG_DUMP
		SerComSendArrayUser(JpegBuffer,sz[1]);
#endif	
		}

		if(sz[0] > 0 )//write First file
		{
			k = i%512; //Continue writing at 512 byte boundary
			i += 512-k; //TODO optimize this
			
			if( sz[0] < OFFLOAD_BUFFER_1SZ)
			{
				i += FlashFileWrite(i,handle,Jpeg_b1,sz[0]);
#ifdef UART_IMG_DUMP
				SerComSendArrayUser(Jpeg_b1,sz[0]);
#endif
			}
			else
			{
				i += FlashFileWrite(i,handle,Jpeg_b1,OFFLOAD_BUFFER_1SZ);//This will automatically end at 512 boundary as it is multiple of 1024
				i += FlashFileWrite(i,handle,Jpeg_b2,sz[0] - OFFLOAD_BUFFER_1SZ);
#ifdef UART_IMG_DUMP
				SerComSendArrayUser(Jpeg_b1,OFFLOAD_BUFFER_1SZ);
				SerComSendArrayUser(Jpeg_b2,sz[0] - OFFLOAD_BUFFER_1SZ);
#endif
			}
		}
		FlashFileBytesWritten = i;
		FlashCloseFile(handle,seq);//Close File Access
		
	}
	//update sequence and filesize variable
	*nseq = seq;
	*fsz = i;
	//Report bytes written and sequence number
	return err;
}
