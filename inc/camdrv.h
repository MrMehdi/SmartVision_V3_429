#ifndef CAMDRV_H__
#define CAMDRV_H__

	#define CAM_NONE						0
	#define CAM1 								1
	#define CAM2 								2
	#define DCMI_DR_ADDRESS     0x50050028
	#include "stm32f4xx.h"

	#define POWER_UP_CLICK_CAM1  (0<<1)
	#define POWER_UP_CLICK_CAM2  (1<<2)
	#define POWER_UP_CLICK_REV	 (2<<1)
	
	typedef struct
	{
		u32 header;
		u8  contrast;
		u8  brightness;
		u8  saturation;
		u8  quality;
		u8  effects;
		u8  lightmode;
		u16 shutter_speed;
		u8  cam_clock;
		u16 cdly;
		u8  drive;		
		u16 extra_lines;
		u8  click_seq;
		u16 start_up_dly;
	}camera_setting;

  extern u8 CamApi_Setup(camera_setting * c);
	extern void Cam_LED(unsigned char enb);
	void Cam_Configure_Peripherals(void);
	u8 Cam_Init_Registers(camera_setting * cs);
	u8 Cam_click_images(u8 clickseq,u16 dly);
	void Cam_Test_3640(void);
	u8 CamApi_Init_Regs(void);
	u8 CamApi_Init_16Regs(void);
#endif

