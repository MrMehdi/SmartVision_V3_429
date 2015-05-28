#ifndef __SCCB_H
#define __SCCB_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"

					  
/* Private define ------------------------------------------------------------*/
#define I2C_PAGESIZE	4

#define VISION_SCCB                        		I2C2
#define VISION_SCCB_CLK                    		RCC_APB1Periph_I2C2

#define VISION_SCCB_SDA_PIN                 	GPIO_Pin_11
#define VISION_SCCB_SDA_GPIO_PORT           	GPIOB
#define VISION_SCCB_SDA_GPIO_CLK            	RCC_AHB1Periph_GPIOB
#define VISION_SCCB_SDA_SOURCE              	GPIO_PinSource11
#define VISION_SCCB_SDA_AF                  	GPIO_AF_I2C2

#define VISION_SCCB_SCL_PIN                 	GPIO_Pin_10
#define VISION_SCCB_SCL_GPIO_PORT           	GPIOB
#define VISION_SCCB_SCL_GPIO_CLK            	RCC_AHB1Periph_GPIOB
#define VISION_SCCB_SCL_SOURCE              	GPIO_PinSource10
#define VISION_SCCB_SCL_AF                  	GPIO_AF_I2C2

/* Maximum Timeout values for flags and events waiting loops. These timeouts are
   not based on accurate values, they just guarantee that the application will 
   not remain stuck if the I2C communication is corrupted.
   You may modify these timeout values depending on CPU frequency and application
   conditions (interrupts routines ...). */  
	 
#define SCCB_VISION_FLAG_TIMEOUT     	 100000

#define OV2640_DEVICE_WRITE_ADDRESS    0x60
#define OV2640_DEVICE_READ_ADDRESS     0x61

#define OV5642_DEVICE_WRITE_ADDRESS		 0x78


#define OV3640_DEVICE_WRITE_ADDRESS		 0x78		
#define OV3640_DEVICE_READ_ADDRESS		 0x79		



#define SCCB_SPEED              			 100000// 40000 //
#define SCCB_SLAVE_ADDRESS7      			 0xFE

/* Private function prototypes -----------------------------------------------*/
void SCCB_GPIO_Config(void);
uint8_t SCCB_Write(uint8_t Addr, uint8_t Data);
uint8_t SCCB_Read(uint8_t Addr, uint8_t *Data);
void SCCBSetAdress(unsigned char add);
uint8_t SCCB_Write16(u16 Reg,u8 Data);
uint8_t SCCB_Read16(u16 Reg, u8 *Data);

#endif 

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
