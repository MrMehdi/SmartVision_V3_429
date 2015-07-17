#include <stdio.h>
#include <string.h>
#include "stm32f4xx.h"
#include "SCCB.h"
#include "camdrv.h"
#include "serialcom.h"
#include "flash.h"


volatile unsigned d_m;
void Delayms(unsigned iv);
extern volatile unsigned jpegCaptureTime;
extern camera_setting cs_flash;
void SysTick_Handler(void)
{
	if(d_m)d_m--;
	jpegCaptureTime++;
}


int main(void)
{
	u8 err=0;
	
	u8 sb[50];
	SysTick_Config(SystemCoreClock / 1000); //1Ms Tick,used for delay,updated on core clock freq changes
	SerComConfig();
#ifdef SERCOM_TEST
	while(1)
	{
		Delayms(1000);
		SerComSendMessageUser("STM DBG PORT\r\n");
		SerComSendStr("NORDIC-STM COM PORT\r\n");
	}
#endif
	SerComSendMessageUser("Initializing Camera I2C\r\n");
	sprintf(sb,"I2C Speed: %d Khz\r\n",SCCB_SPEED/1000);
	SerComSendMessageUser(sb);
	SCCB_GPIO_Config();
	
	SerComSendMessageUser("Initializing Camera Peripherals\r\n");
	Cam_Configure_Peripherals();
	
	SerComSendMessageUser("Initializing Flash\r\n");
	if(Flash_Init_SPI() != 0)
		err |= ERROR_HARD_FLASH;
	
	SerComSendMessageUser("\r\nReading Camera Settings from Flash\r\n");
	Flash_Read(FLASH_CAM_SETTING_ADD,(void*)&cs_flash,sizeof(cs_flash));//read camera settings from internal flash
	if (cs_flash.header != 0x01ABCDEF )
	{
		SerComSendMessageUser("Default Settings loaded\r\n");
		CamApi_load_default_settings(&cs_flash);
		cs_flash.header = 0x01ABCDEF;
		SerComSendMessageUser("Storing Settings to Flash\r\n");
		Flash_Write(FLASH_CAM_SETTING_ADD,(void*)&cs_flash,sizeof(cs_flash));
	}
	CamApi_display_settings(&cs_flash);
	
	memcpy((void*)&imgdebug.cs,(void*)&cs_flash,sizeof(cs_flash));//copy image header information required during file storage
	
	SerComSendMessageUser("\r\nReading File Table System\r\n");
	Flash_Read(FTABLE_ADDRESS,(void*)&file_sys,sizeof(file_sys));
	if ( file_sys.header  != 0x01ABCDEF)
	{
		SerComSendMessageUser("File Table System Corrupt!\r\n");
		Flash_Create_FS();
	}
	Flash_Dump_filetable();
	
	SerComSendMessageUser("Initializing Camera registers\r\n");
	Cam_Init_Power();
	//err |= Cam_Init_Registers(&cs);//Initialize both camera registers to prepare for click,both cameras in power down state on return
	SerComSendMessageUser("Ready for operation\r\n");
	
	SerComSendRsp(0,&err,1);    
	
	while(1)
	{
		SerComProcess();	
	}
	
}

void Delayms(unsigned iv)
{
	d_m = iv;
	while(d_m);//reduced every 1 Msec in Systick Interrupt
}
