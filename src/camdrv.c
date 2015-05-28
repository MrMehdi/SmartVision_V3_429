#include "misc.h"
#include <stdio.h>
#include "SCCB.h"
#include "OV2640.h"
#include "ov2640_regs.h"
#include "camdrv.h"
#include "sram.h"
#include "serialcom.h"
#include "flash.h"

#define CAMERA_IMGCAPTURE_TIMEOUT 500
#define EXP_TIME_MS 							5
#define MAX_IMAGE_SZ							256*1024
#define TOTAL_IMAGES_IN_SRAM			2
#define JPEG_BUFFER_SZ						1024*96

extern void Delayms(unsigned iv);
extern volatile unsigned char jpegCaptureDoneITFLG;
extern volatile unsigned jpegCaptureTime;
extern volatile unsigned jpegTotalTime;


#ifdef IM_MODE_INTRAM
	unsigned char JpegBuffer[JPEG_BUFFER_SZ];
#endif

u16 click_dly;

void Cam_LED(unsigned char enb)
{
	if(enb == ENABLE)
	{
		GPIO_SetBits(GPIOC, GPIO_Pin_0);
	}
	else
	{
		GPIO_ResetBits(GPIOC, GPIO_Pin_0);
	}
}
void Cam_Power_Down(unsigned char cam,unsigned char enb)
{
	if(cam == CAM1)
	{
		if(enb == ENABLE)
		{
			GPIO_SetBits(GPIOA, GPIO_Pin_11);
		}
		else
		{
			GPIO_ResetBits(GPIOA, GPIO_Pin_11);
		}
	}
	if(cam == CAM2)
	{
		if(enb == ENABLE)
		{
			GPIO_SetBits(GPIOA, GPIO_Pin_12);
		}
		else
		{
			GPIO_ResetBits(GPIOA, GPIO_Pin_12);
		}
	}
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
#ifdef IM_MODE_SRAM
	DMA_InitStructure.DMA_Memory0BaseAddr = 0x64000000;
#endif
#ifdef IM_MODE_INTRAM	
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)JpegBuffer;	
#endif
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
void Cam_IO_Configure(void)
{

	GPIO_InitTypeDef GPIO_InitStructure;
	
	/* Enable GPIOs clocks */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);		
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;  
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;  
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	/*
Camera 1(UP)
CAM1RESET		PA9
CAM1PWDN		PA11
	
Camera 2(LOW)
CAM2RESET		PA10
CAM2PWDN		PA12

PC0 Camera LED


*/

	GPIO_SetBits(GPIOA, GPIO_Pin_11);//Activate Power Down
	GPIO_SetBits(GPIOA, GPIO_Pin_12);
	
	GPIO_ResetBits(GPIOB, GPIO_Pin_3);//Activate Reset
	GPIO_ResetBits(GPIOB, GPIO_Pin_4);
	
	GPIO_ResetBits(GPIOC, GPIO_Pin_0);
	
	

}

void Cam_Enable_Clock(unsigned char cam,unsigned char enb)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_BaseStruct;
	TIM_OCInitTypeDef TIM_OCStruct;
	GPIO_InitTypeDef GPIO_InitStruct;

	
	if(cam == CAM1)
	{
		if(enb == 1)
		{
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
			TIM_BaseStruct.TIM_Prescaler = 0;
			/* Count up */
			TIM_BaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
			TIM_BaseStruct.TIM_Period = 6; /* 10kHz PWM *///6
			TIM_BaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
			TIM_BaseStruct.TIM_RepetitionCounter = 0;
			/* Initialize TIM4 */
			TIM_TimeBaseInit(TIM2, &TIM_BaseStruct);
			/* Start count on TIM4 */
			TIM_Cmd(TIM2, ENABLE);
			TIM_OCStruct.TIM_OCMode = TIM_OCMode_PWM1;
			TIM_OCStruct.TIM_OutputState = TIM_OutputState_Enable;
			TIM_OCStruct.TIM_OCPolarity = TIM_OCPolarity_Low;
			TIM_OCStruct.TIM_Pulse = 3; /* 50% duty cycle *///3
			TIM_OC2Init(TIM2, &TIM_OCStruct);
			TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);			
			RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
			/* Alternating functions for pins */
			GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_TIM2);
			/* Set pins */
			GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;
			GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
			GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
			GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
			GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
			GPIO_Init(GPIOA, &GPIO_InitStruct);			
			
		}
		else
		{
			return;
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, DISABLE);
			TIM_Cmd(TIM2, DISABLE);
			GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;
			GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;
			GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
			GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
			GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
			GPIO_Init(GPIOA, &GPIO_InitStruct);	
			
		}
	}
	if(cam == CAM2)
	{
		if(enb == 1)
		{
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
			RCC_MCO1Config(RCC_MCO1Source_HSE, RCC_MCO1Div_1);// 16MHZ
		}
		else
		{
			return;
			RCC_ClockSecuritySystemCmd(ENABLE);
			/* Enable GPIOs clocks */
			RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);			
			/* Configure MCO (PA8) */
			GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
			GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
			GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;  //UP
			GPIO_Init(GPIOA, &GPIO_InitStructure);
		}
	}
}


void Cam_Configure_Peripherals(void)
{

	
	Cam_IO_Configure();
	Cam_DCMI_Config();
	Cam_DMA_Init();
	
	//Cam_Enable_Clock(CAM1,1);			//enable clock
	//Cam_Enable_Clock(CAM2,1);			//enable clock
	//while(1);
}

//void Cam_Test_3640(void)
//{
//	SCCBSetAdress(OV3640_DEVICE_WRITE_ADDRESS);
//	SerComSendMessageUser("Initializing OV3640\r\n");
//	while(1);
//}

u8 Cam_Init_Registers(camera_setting * cs)
{	
	u8 err=0;
	SCCBSetAdress(OV2640_DEVICE_WRITE_ADDRESS);
	//SerComSendMessageUser("Hard Reseting Cameras\r\n");
	Cam_Power_Down(CAM1,DISABLE);	//Deactivate power down
	Cam_Power_Down(CAM2,DISABLE);
//	GPIO_ResetBits(GPIOB, GPIO_Pin_3); 
//	GPIO_ResetBits(GPIOB, GPIO_Pin_4);
//	Delayms(100);
	GPIO_SetBits(GPIOB, GPIO_Pin_3); //Deactivate Reset from camera
	GPIO_SetBits(GPIOB, GPIO_Pin_4);
	
	//Delayms(100);
	
	//SerComSendMessageUser("Configure Camera 1\r\n");
		
	//Configure camera 1
	Cam_Power_Down(CAM1,DISABLE);	//Deactivate power down
	Cam_Power_Down(CAM2,ENABLE);
	Cam_Enable_Clock(CAM1,1);			//enable clock
	Delayms(5);
	
	if(CamApi_Setup(cs) > 0)				//configure camera registers
	{
		SerComSendMessageUser("Camera 1 did not init correctly\r\n");
		err |= (1<<ERROR_HARD_CAM1);
	}
	
	Cam_Enable_Clock(CAM1,0);			//disable clock
	
	//SerComSendMessageUser("Configure Camera 2\r\n");
	
	//Configure camera 2
	Cam_Power_Down(CAM1,ENABLE);  //Deactivate power down
	Cam_Power_Down(CAM2,DISABLE);
	Cam_Enable_Clock(CAM2,1);
	Delayms(5);
	
	if(CamApi_Setup(cs) > 0)			//configure camera registers
	{
		SerComSendMessageUser("Camera 2 did not init correctly\r\n");
		err |= (1<<ERROR_HARD_CAM2);
	}
		
	Cam_Enable_Clock(CAM2,0);			//disable clock
	
	//SerComSendMessageUser("Enter Standby on both cameras\r\n");
	
	Cam_Power_Down(CAM1,ENABLE);  //Activate power down
	Cam_Power_Down(CAM2,ENABLE);
	
	return err;
}


u32 GetImageSize(u8 * buf , u32 len)
{
	unsigned i;
	
	if (buf[0] == 0xff && buf[1] == 0xd8)
	{
		for( i = 2;i < len ;i++)
		{
			if(buf[i] == 0xff && buf[i+1] == 0xd9)
				return i;
		}
	}
	return 0;
}

u8 Cam_Get_Image(void)
{
	jpegCaptureDoneITFLG = 0;
	jpegCaptureTime	= 0;
	
	Delayms(EXP_TIME_MS);										//Fail-safe exposure time to 5ms	
	DMA_Cmd(DMA2_Stream1, ENABLE);				  													  //Start DMA
	DCMI_Cmd(ENABLE);												//Enable DCMI	
	DCMI_CaptureCmd(ENABLE);								//Start Capture of Image
	
	while(jpegCaptureDoneITFLG ==0){													//jpegCaptureDoneITFLG set in DCMI IRQ
	
		if(jpegCaptureTime > CAMERA_IMGCAPTURE_TIMEOUT)					//Wait for timeout
		{
			jpegTotalTime += CAMERA_IMGCAPTURE_TIMEOUT;						//Camera image capture failed, timeout
			SerComSendMessageUser("Camera Capture Timed out\r\n");
			return 0;
		}
	}
	jpegTotalTime += jpegCaptureTime;				//store total capture time
	DMA_Cmd(DMA2_Stream1, DISABLE);				  //Stop DMA
	DCMI_Cmd(DISABLE);																									
	DCMI_CaptureCmd(DISABLE);
	
	return 1;

}

u8 Cam_trigger_image(u8 camno,u16 dly)
{
	u8 err=0;
	u8 sb[20];
	
	Cam_LED(ENABLE);												//Indicate camera capture started
	Cam_Power_Down(camno,DISABLE);					//Disable power down signal
	Cam_Enable_Clock(camno,ENABLE);					//Start clock to camera hardware
	
	Delayms(dly);														//Startup delay on power down release
	if(Cam_Get_Image()==0){
		err = 1;					//capture image on DCMI
		sprintf(sb,"CAM%d capture failed\r\n",camno);
		SerComSendMessageUser(sb); 
	}
	Cam_Enable_Clock(camno,DISABLE); 				//Put camera back in sleep mode
	Cam_Power_Down(camno,ENABLE);		 				//Disable clock to save MCU power	
	Cam_LED(DISABLE);												//Indicate camera trigger completed
	return err;
}


u8 Cam_click_images(u8 cseq,u16 dly)
{
	u8 res[2],sb[30],cams[2];
	u32 esz,tmp,dtim;
	
	
	res[0] = res[1] = 0;
	cams[0] = CAM_NONE;cams[1] = CAM_NONE;															//Deselect both cameras
	jpegTotalTime = 0;																									//Reset timer for total image capture time
	
	if(((cseq & POWER_UP_CLICK_CAM1) == POWER_UP_CLICK_CAM1) && 
		 ((cseq & POWER_UP_CLICK_CAM2) == POWER_UP_CLICK_CAM2)) 					//Check if we have to click two images
	{
		if(cseq & POWER_UP_CLICK_REV){cams[0] = CAM2;cams[1] = CAM1;}			//Load the correct image click sequence
			else{cams[0] = CAM1;cams[1] = CAM2;}
	}
	else																																//Check if one camera click is needed
	{
		if(cseq & POWER_UP_CLICK_CAM1)cams[0] = CAM1;											//Load the appropriate camera for click image
			else if (cseq & POWER_UP_CLICK_CAM2)cams[0] = CAM2;						
	}
	
	Cam_Power_Down(CAM1,ENABLE);																				//Power down both camera
	Cam_Power_Down(CAM2,ENABLE);
	
	
	if(cams[0] != CAM_NONE)
		res[0] = Cam_trigger_image(cams[0],dly);													//take the first image	
	
	dtim = jpegTotalTime;
	
	if(cams[1] != CAM_NONE){
																								
		esz = GetImageSize(JpegBuffer,JPEG_BUFFER_SZ);										//Calculate the size of image using SOF and EOF for JPEG
		SRAM_WriteBuffer((void*)&JpegBuffer[0],0,esz/2 + 10);							//Store previouse image to External SRAM
		JpegBuffer[0] = 0;JpegBuffer[1] = 0;															//Clear SOF from Internal RAM
		Cam_DMA_Init();																										//Reset DMA to capture image from start of RAM		
		res[1] = Cam_trigger_image(cams[1],dly);													//take the second image
	}
 																																			
	if(cams[0] != CAM_NONE)																							//check if image was clicked at all
	{																																		//First store data from internal ram to flash
		tmp = GetImageSize(JpegBuffer,JPEG_BUFFER_SZ);								  	//Get image size	
		Flash_writefile_jpeg_intram(JpegBuffer,tmp);											//Store to Flash and update flash table
	}
	
	if(cams[1] != CAM_NONE)																							//check if image was clicked at all
	{																																		//First store data from internal ram to flash
		SRAM_ReadBuffer((void*)JpegBuffer,0, esz/2 + 10);									//Load this data to internal Ram
		Flash_writefile_jpeg_intram(JpegBuffer,esz);											//Store to Flash and update flash table									
	}
	
	
	DMA_Cmd(DMA2_Stream1, DISABLE);																			//Disable DMA
	Cam_DCMI_Config();
	Cam_DMA_Init();
	
	sprintf(sb,"Cam1:%d err:%d Cam2:%d err%d \r\n", jpegTotalTime-dtim,res[0],jpegTotalTime,res[1]);
	SerComSendMessageUser(sb); 
	
#ifdef FLASH_DBG		
	Flash_Dump_filetable();			
#endif	
	
	return res[0]+res[1];
}

u32 Cam_get_image_sz(void)
{
	u8 JpegBuffer[8];
	u32 i,j=0;
	SRAM_ReadBuffer((u16*)JpegBuffer,0, 4);
	
	if((JpegBuffer[0]==0xFF)&&(JpegBuffer[1]==0xD8)){//Check if image exists,image will always start here irrespective.

		for( i=0;i<MAX_IMAGE_SZ;i++ ){
				if( j >= TOTAL_IMAGES_IN_SRAM)
					break;

						
			SRAM_ReadBuffer((u16*)JpegBuffer,i, 4);
			
			if((JpegBuffer[0]==0xFF)&&(JpegBuffer[1]==0xD9)){
				i += 2;
				j++;
			}
			else if((JpegBuffer[1]==0xFF)&&(JpegBuffer[2]==0xD9)){
				i += 3;
				j++;
			}	
			else if((JpegBuffer[2]==0xFF)&&(JpegBuffer[3]==0xD9)){
				i += 4;
				j++;
			}
			else if((JpegBuffer[0]==0xFF)&&(JpegBuffer[1]==0xFF)&& //Empty RAM section
							(JpegBuffer[2]==0xFF)&&(JpegBuffer[3]==0xFF)){
				j++;
				break;
			}
			else{					
						i += 4;
			}
		}
	if(j == 0)
	{
		SerComSendMessageUser("No images found\r\n");
		return 0;
	}
	 return i;
		
	}
	return 0;
}
