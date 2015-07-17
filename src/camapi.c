#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sccb.h"
#include "camdrv.h"
#include "ov2640_regs.h"
#include "camapi.h"

extern void Delayms(unsigned iv);
u8 camr[10];

#define I2CFAIL 1
#define I2COK		0


u8 CamApi_WriteTable(const uint8_t (*regs)[2])
{
	u32 i=0;
	u8 ret = 0;
	/* Write initial regsiters */
	while (regs[i][0]) {
			ret |= SCCB_Write(regs[i][0], regs[i][1]);
			i++;
	}
	return ret;
}

u8 CamApi_WriteTable16(const struct sensor_reg reglist[])
{
	u8 ret = 0;
	const struct sensor_reg *next = reglist;
	/* Write initial regsiters */
	while ((next->reg != 0xffff) | (next->val != 0xff)) {
			ret |= SCCB_Write16(next->reg, next->val);
			next++;
	}
	return ret;
}


u8 	CamApi_Set_Quality(unsigned char val)
{
	u8 ret;
		/* Switch to DSP register bank */
	ret |= SCCB_Write(BANK_SEL, BANK_SEL_DSP);

	/* Write QS register */
	ret |= SCCB_Write(QS, val);

	
	return ret;
}
 		
u8  CamApi_Set_Gain(unsigned char val)
{
	u8 ret;
	ret |= SCCB_Write(BANK_SEL, BANK_SEL_SENSOR);
	ret |= SCCB_Write(GAIN,val);
	return ret; 
}
u8 CamApi_Set_GainCeiling(enum sensor_gainceiling gainceiling)
{
    u8 ret =0;

    /* Switch to SENSOR register bank */
    ret |= SCCB_Write(BANK_SEL, BANK_SEL_SENSOR);

    /* Write gain ceiling register */
    ret |= SCCB_Write(COM9, COM9_AGC_SET(gainceiling));

    return ret;
}
u8 	CamApi_Set_Shutter(unsigned short speed)
{
	u8 ret=0;
	unsigned char v;
	unsigned short temp16;
		//Shutter = (reg0x45 & 0x3f) << 10 + reg0x10<<2 + (reg0x04 & 0x03);
	
  /* Switch to DSP register bank */
  ret |= SCCB_Write(BANK_SEL, BANK_SEL_SENSOR);

	
	//Set register 04 bits 0,1
	SCCB_Read(0x04, &v);
	v &= 0xFC;
	v |= (unsigned char)speed & 0x3;
	ret |= SCCB_Write(0x04, v);

	//Set register 10(AEC) bits 2-9
	temp16 = speed>>2;
	ret |= SCCB_Write(AEC, (unsigned char)(temp16));
	
	
	//Set register(45) bit 15:10
	ret |= SCCB_Read(REG45, &v);
	v &= 0x3f;
	temp16 = speed>>10;
	ret |= SCCB_Write(REG45,(unsigned char)temp16);
	
	return ret;
}
u8 CamApi_Set_Contrast(int level)
{
    int i;
	  u8 ret=0;

		if (level > NUM_CONTRAST_LEVELS-1) {
        level = NUM_CONTRAST_LEVELS-1;
    }

    /* Switch to DSP register bank */
    ret |= SCCB_Write(BANK_SEL, BANK_SEL_DSP);

    /* Write contrast registers */
    for (i=0; i<sizeof(contrast_regs[0])/sizeof(contrast_regs[0][0]); i++) {
        ret |= SCCB_Write(contrast_regs[0][i], contrast_regs[level+1][i]);
    }

    return ret;
}

u8 CamApi_Set_Brightness(int level)
{
    int i;
	  u8 ret=0;

		if (level > NUM_BRIGHTNESS_LEVELS-1) {
        level = NUM_BRIGHTNESS_LEVELS-1;
    }

    /* Switch to DSP register bank */
    ret |= SCCB_Write(BANK_SEL, BANK_SEL_DSP);

    /* Write brightness registers */
    for (i=0; i<sizeof(brightness_regs[0])/sizeof(brightness_regs[0][0]); i++) {
        ret |= SCCB_Write(brightness_regs[0][i], brightness_regs[level+1][i]);
    }

    return ret;
}

u8 CamApi_Set_Saturation(int level)
{
    int i;
		u8 ret=0;

    if (level > NUM_SATURATION_LEVELS-1) {
        level = NUM_SATURATION_LEVELS-1;
    }

    /* Switch to DSP register bank */
    ret |= SCCB_Write(BANK_SEL, BANK_SEL_DSP);

    /* Write contrast registers */
    for (i=0; i<sizeof(saturation_regs[0])/sizeof(saturation_regs[0][0]); i++) {
        ret |= SCCB_Write(saturation_regs[0][i], saturation_regs[level+1][i]);
    }

    return ret;
}

u8 CamApi_Set_Effects(int level)
{
    u8 i,ret=0;

    if (level > NUM_SPECIAL_EFFECTS-1) {
				level = NUM_SPECIAL_EFFECTS-1;
    }

    /* Switch to DSP register bank */
    ret |= SCCB_Write(BANK_SEL, BANK_SEL_DSP);

    /* Write contrast registers */
    for (i=0; i<sizeof(effects_regs[0])/sizeof(effects_regs[0][0]); i++) {
        ret |= SCCB_Write(effects_regs[0][i], effects_regs[level+1][i]);
    }

    return ret;
}



u8 CamApi_Set_Lightmode(unsigned char mode)
{
	u8 ret=0;
	/* Switch to DSP register bank */
	ret |= SCCB_Write(BANK_SEL, BANK_SEL_DSP);
	
	if( mode == LIGHT_MODE_AUTO )
	{
		ret |= SCCB_Write(0xc7, 0x00); //AWB on
	}
	else if( mode == LIGHT_MODE_SUNNY)
	{
		ret |= SCCB_Write(0xc7, 0x40); //AWB off
		ret |= SCCB_Write(0xcc, 0x5e);
		ret |= SCCB_Write(0xcd, 0x41);
		ret |= SCCB_Write(0xce, 0x54);
	}
	else if( mode == LIGHT_MODE_CLOUDY)
	{
		ret |= SCCB_Write(0xc7, 0x40); //AWB off
		ret |= SCCB_Write(0xcc, 0x65);
		ret |= SCCB_Write(0xcd, 0x41);
		ret |= SCCB_Write(0xce, 0x4f);
	}
	else if(mode == LIGHT_MODE_OFFICE)
	{
		ret |= SCCB_Write(0xc7, 0x40); //AWB off
		ret |= SCCB_Write(0xcc, 0x52);
		ret |= SCCB_Write(0xcd, 0x41);
		ret |= SCCB_Write(0xce, 0x66);
	}
	else if(mode == LIGHT_MODE_HOME)
	{
		ret |= SCCB_Write(0xc7, 0x40); //AWB off
		ret |= SCCB_Write(0xcc, 0x42);
		ret |= SCCB_Write(0xcd, 0x3f);
		ret |= SCCB_Write(0xce, 0x71);
	}
		
return ret;
}
u8 CamApi_Set_Register(u8 bank,u8 reg,u8 val)
{
	u8 ret=0;
	/* Switch to DSP register bank */
	ret |= SCCB_Write(BANK_SEL, bank);
	ret |= SCCB_Write(reg, val);
	
	return ret;
}
u8 CamApi_Init_16Regs(void)
{
	u8 ret=0;
	u8 reg_val;
	
	ret |= SCCB_Write16(0x3008, 0x80); //Reset Sensr
	/* delay n ms */
	Delayms(1000); 
	

	camr[0] = 0;
	camr[1] = 0;
	SCCB_Read16(OV5642_CHIPID_HIGH, &camr[0]);
  SCCB_Read16(OV5642_CHIPID_LOW, &camr[1]);
	
	if((camr[0] != 0x56) || (camr[1] != 0x42))
		return(I2CFAIL);									
	
	

	ret |= CamApi_WriteTable16(OV5642_1080P_Video_setting);
	ret |= SCCB_Read16(0x3818,&reg_val);
	reg_val = (reg_val | 0x20) & 0xBf;
	ret |= SCCB_Write16(0x3818, reg_val);
	ret |= SCCB_Read16(0x3818, &reg_val);
	ret |= SCCB_Read16(0x3621,&reg_val);
	ret |= SCCB_Write16(0x3621, reg_val | 0x20);
  ret |= SCCB_Read16(0x3621, &reg_val);
	return ret;
}

u8 CamApi_Init_Regs(void)
{
	u8 r = 0;
	
	r |= SCCB_Write(BANK_SEL, BANK_SEL_SENSOR);
	r |= SCCB_Write(COM7, COM7_SRST);
//	/* delay n ms */
	Delayms(5); 
	camr[0] = 0;
	r |= SCCB_Write(0xFF, 0x01);//SELECT CMOS 
	if(SCCB_Read(0x0A,camr)!=0)//READ PID
		return(I2CFAIL);									//Camera did not respond, hardware failure
	
  if(camr[0] != 0x26)
		return(I2CFAIL);
	
	
	r |= CamApi_WriteTable(AR_OV2640_JPEG_INIT);
	r |= CamApi_WriteTable(AR_OV2640_YUV422);
	r |= CamApi_WriteTable(AR_OV2640_JPEG);	
	r |= CamApi_Set_Register(BANK_SEL_SENSOR,COM10, 0);//PCLK rising edge, HREF output positive,VSYNC Polarity positive
	
	r |= CamApi_WriteTable(AR_OV2640_1600x1200_JPEG);
	

	
	r |= CamApi_Set_Contrast(3);
 	r |= CamApi_Set_Brightness(4);
 	r |= CamApi_Set_Saturation(3);
 	r |= CamApi_Set_Quality(10);
  r |= CamApi_Set_Lightmode(LIGHT_MODE_OFFICE);
	r |= CamApi_Set_Effects(SPECIAL_EFFECT_NORMAL);
	r |= CamApi_WriteTable(DS_CAP_7_5FPS);
  r |= CamApi_Set_Gain(GAIN_2X);//1-32
 	r |= CamApi_Set_Shutter(800);	//default 312	-
	
	r |= CamApi_Set_Register(BANK_SEL_SENSOR,ADDVSL,(u8)0);
	r |= CamApi_Set_Register(BANK_SEL_SENSOR,ADDVSH,(u8)0);
	r |= CamApi_Set_Register(BANK_SEL_SENSOR,COM2,COM2_OUT_DRIVE_3x);
  r |= CamApi_Set_Register(BANK_SEL_SENSOR,CLKRC,0x00);//enable internal multiplier(24 mhz clock generates about 18 Mhz piXCLK
	
	//r |= CamApi_Set_Register(BANK_SEL_DSP,0xda,0x12);//HREF = VSYNC
	
	//r |= CamApi_Set_Register(BANK_SEL_SENSOR,COM3,0x3C | 0x01); //signle frame
		
	//r |= CamApi_Set_Register(BANK_SEL_DSP,R_DVP_SP,R_DVP_SP_AUTO_MODE | 0x00);
	//r |= CamApi_Set_Register(BANK_SEL_SENSOR,COM4,0xb3);//High impedance data lines
	//CamApi_Set_Register(BANK_SEL_SENSOR,COM7,COM7_COLOR_BAR);//Enable Color bar
	r |= CamApi_Set_Register(BANK_SEL_SENSOR,COM7,COM7_RES_UXGA);//Zoom disabled
	r |= CamApi_Set_Register(BANK_SEL_SENSOR,IMAGE_MODE, IMAGE_MODE_JPEG_EN|IMAGE_MODE_YUV422);



	return(r);
	
}

void CamApi_load_default_settings(camera_setting * cs)
{
	cs->brightness = 3;
	cs->contrast   = 3;
	cs->saturation = 3;
	cs->shutter_speed = 400;
	cs->quality		= 7;
	cs->cam_clock = 0x02;
	cs->lightmode = LIGHT_MODE_OFFICE;
	cs->effects 	= SPECIAL_EFFECT_NORMAL;
	cs->drive			= COM2_OUT_DRIVE_1x;
	cs->cdly = 5;
	cs->extra_lines = 0;
	cs->click_seq = POWER_UP_CLICK_CAM1 | POWER_UP_CLICK_CAM2 | POWER_UP_CLICK_REV;
}

void CamApi_display_settings(camera_setting *cs)
{
	u8 sb[50];

	SerComSendMessageUser("Camera Settings:\r\n");
	sprintf((void*)sb,"Contrast:%d\t",cs->contrast);
	SerComSendMessageUser(sb);
	sprintf((void*)sb,"Brightness:%d\t",cs->brightness);
	SerComSendMessageUser(sb);
	sprintf((void*)sb,"Saturation:%d\t",cs->saturation);
	SerComSendMessageUser(sb);
	sprintf((void*)sb,"Quality:%d\t",cs->quality);
	SerComSendMessageUser(sb);
	sprintf((void*)sb,"Effects:%d\t",cs->effects);
	SerComSendMessageUser(sb);
	sprintf((void*)sb,"LightMode:%d\t",cs->lightmode);
	SerComSendMessageUser(sb);
	sprintf((void*)sb,"Shutter Speed:%d\t",cs->shutter_speed);
	SerComSendMessageUser(sb);
	sprintf((void*)sb,"Clock Div:%d\t",cs->cam_clock);
	SerComSendMessageUser(sb);
	sprintf((void*)sb,"Click Delay:%d\t",cs->cdly);
	SerComSendMessageUser(sb);
	sprintf((void*)sb,"Drive Factor:%d\r\n",cs->drive);
	SerComSendMessageUser(sb);
	
}

