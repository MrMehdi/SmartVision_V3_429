
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "SCCB.h"

__IO uint32_t  DCMI_TIMEOUT_MAX = SCCB_VISION_FLAG_TIMEOUT;

u8 sccbadd=0;

void SCCBSetAdress(unsigned char add)
{
	sccbadd = add;
}



/*******************************************************************************
* Function Name  : SCCB_GPIO_Config
* Description    : 
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void SCCB_GPIO_Config(void)
{
	I2C_InitTypeDef  SCCB_InitStructure;
	GPIO_InitTypeDef  GPIO_InitStructure; 
	
	RCC_AHB1PeriphClockCmd(VISION_SCCB_SDA_GPIO_CLK | VISION_SCCB_SCL_GPIO_CLK,ENABLE);
	
	RCC_APB1PeriphClockCmd(VISION_SCCB_CLK,ENABLE);
	
	GPIO_PinAFConfig(VISION_SCCB_SDA_GPIO_PORT, VISION_SCCB_SDA_SOURCE, VISION_SCCB_SDA_AF);
	GPIO_PinAFConfig(VISION_SCCB_SCL_GPIO_PORT, VISION_SCCB_SCL_SOURCE, VISION_SCCB_SCL_AF);
	
	GPIO_InitStructure.GPIO_Pin =  VISION_SCCB_SDA_PIN | VISION_SCCB_SCL_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
		
	I2C_DeInit(VISION_SCCB);
	SCCB_InitStructure.I2C_Mode = I2C_Mode_I2C;
	SCCB_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	SCCB_InitStructure.I2C_OwnAddress1 = SCCB_SLAVE_ADDRESS7;
	SCCB_InitStructure.I2C_Ack = I2C_Ack_Enable;
	SCCB_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	SCCB_InitStructure.I2C_ClockSpeed = SCCB_SPEED;
	
	I2C_Init(VISION_SCCB, &SCCB_InitStructure);
	I2C_Cmd(VISION_SCCB, ENABLE);

	
	I2C_AcknowledgeConfig(VISION_SCCB, ENABLE);	
}

/**
  * @brief  Writes a byte at a specific Camera register
  * @param  Device: write address.
  * @param  Addr:  register address. 
  * @param  Data: data to be written to the specific register 
  * @retval 0x00 if write operation is OK. 
  *         0xFF if timeout condition occured (device not connected or bus error).
  */
uint8_t SCCB_Write(uint8_t Reg, uint8_t Data)
{
  uint32_t timeout = DCMI_TIMEOUT_MAX;
  
  /* Generate the Start Condition */
  I2C_GenerateSTART(VISION_SCCB, ENABLE);

  /* Test on I2C2 EV5 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_MODE_SELECT))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  }
   /*-----------------------------------------------------------------------------------*/
  /* Send DCMI selcted device slave Address for write */
  I2C_Send7bitAddress(VISION_SCCB, sccbadd , I2C_Direction_Transmitter);
 // for(i = 0; i< 0xff;i++);
  /* Test on I2C2 EV6 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  }
   /*-----------------------------------------------------------------------------------*/
  /* Send I2C2 location address LSB */
  I2C_SendData(VISION_SCCB, (uint8_t)(Reg));
  //for(i = 0; i< 0xff;i++);
  /* Test on I2C2 EV8 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  }
   /*-----------------------------------------------------------------------------------*/
  /* Send Data */
  I2C_SendData(VISION_SCCB, Data);    
  //for(i = 0; i< 0xff;i++);
  /* Test on I2C2 EV8 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  }  
   /*-----------------------------------------------------------------------------------*/
  /* Send I2C2 STOP Condition */
  I2C_GenerateSTOP(VISION_SCCB, ENABLE);
  //for(i = 0; i< 0xff;i++);
  /* If operation is OK, return 0 */
  return 0;
}

/**
  * @brief  Writes a byte at a specific Camera register
  * @param  Device: write address.
  * @param  Addr:  register address. 
  * @param  Data: data to be written to the specific register 
  * @retval 0x00 if write operation is OK. 
  *         0xFF if timeout condition occured (device not connected or bus error).
  */
uint8_t SCCB_Write16(u16 Reg, u8 Data)
{
  uint32_t i,timeout = DCMI_TIMEOUT_MAX;
  
  /* Generate the Start Condition */
  I2C_GenerateSTART(VISION_SCCB, ENABLE);

  /* Test on I2C2 EV5 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_MODE_SELECT))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  }
	
  /*-----------------------------------------------------------------------------------*/
  /* Send DCMI selcted device slave Address for write */
  I2C_Send7bitAddress(VISION_SCCB, sccbadd , I2C_Direction_Transmitter);
  for(i = 0; i< 0xff;i++);
  /* Test on I2C2 EV6 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  }
	
	
  /*-----------------------------------------------------------------------------------*/
  /* Send I2C2 location address MSB8 */
  I2C_SendData(VISION_SCCB, (uint8_t)(Reg>>8));
  //for(i = 0; i< 0xff;i++);
  /* Test on I2C2 EV8 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  }
  /*-----------------------------------------------------------------------------------*/
	
	/*-----------------------------------------------------------------------------------*/
  /* Send I2C2 location address LSB8 */
  I2C_SendData(VISION_SCCB, (uint8_t)(Reg&0x00FF));
  for(i = 0; i< 0xff;i++);
  /* Test on I2C2 EV8 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  }
  /*-----------------------------------------------------------------------------------*/
  /* Send Data */
  I2C_SendData(VISION_SCCB, Data);    
  //for(i = 0; i< 0xff;i++);
  /* Test on I2C2 EV8 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  }  
  /*-----------------------------------------------------------------------------------*/
  /* Send I2C2 STOP Condition */
  I2C_GenerateSTOP(VISION_SCCB, ENABLE);
 for(i = 0; i< 0xff;i++);
  /* If operation is OK, return 0 */
	//Delayms(1); 
  return 0;
}



/**
  * @brief  Reads a byte from a specific Camera register 
  * @param  Device: write address.
  * @param  Addr:  register address. 
  * @retval data read from the specific register or 0xFF if timeout condition 
  *         occured. 
  */
uint8_t SCCB_Read(uint8_t Reg, uint8_t *Data)
{
  uint32_t timeout = DCMI_TIMEOUT_MAX;
	u32 i;
    /* Clear AF flag if arised */
  I2C2->SR1 |= (uint16_t)0x0400;

  /* Generate the Start Condition */
  I2C_GenerateSTART(VISION_SCCB, ENABLE);

  /* Test on I2C2 EV5 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_MODE_SELECT))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  } 
  /*-----------------------------------------------------------------------------------*/
  /* Send DCMI selcted device slave Address for write */
  I2C_Send7bitAddress(VISION_SCCB, sccbadd | 0x01 , I2C_Direction_Transmitter);
 
  /* Test on I2C2 EV6 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  } 
  /*-----------------------------------------------------------------------------------*/
  /* Send I2C2 location address LSB */
  I2C_SendData(VISION_SCCB, (uint8_t)(Reg));
  for(i = 0; i< 0xffff;i++);
  /* Test on I2C2 EV8 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  } 
  
  /* Clear AF flag if arised */
  I2C2->SR1 |= (uint16_t)0x0400;
  /*-----------------------------------------------------------------------------------*/    
  /* Prepare Stop after receiving data */
  I2C_GenerateSTOP(VISION_SCCB, ENABLE);

  /*-----------------------------------------------------------------------------------*/
  /* Generate the Start Condition */
  I2C_GenerateSTART(VISION_SCCB, ENABLE);
  
  /* Test on I2C2 EV6 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_MODE_SELECT))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  } 
  /*-----------------------------------------------------------------------------------*/
  /* Send DCMI selcted device slave Address for write */
  I2C_Send7bitAddress(VISION_SCCB, sccbadd | 0x01, I2C_Direction_Receiver);
  for(i = 0; i< 0xffff;i++);
  /* Test on I2C2 EV6 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  }
      
  /*-----------------------------------------------------------------------------------*/
  /* Prepare an NACK for the next data received */
  I2C_AcknowledgeConfig(VISION_SCCB, DISABLE);

  /* Test on VISION_SCCB EV7 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_BYTE_RECEIVED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  } 
  /*-----------------------------------------------------------------------------------*/    
  /* Prepare Stop after receiving data */
  I2C_GenerateSTOP(VISION_SCCB, ENABLE); 

  /*-----------------------------------------------------------------------------------*/
  /* Receive the Data */
  *Data = I2C_ReceiveData(VISION_SCCB);
  /* Clear AF flag if arised */
  I2C2->SR1 |= (uint16_t)0x0400;

  /* return the read data */
  return 0;
}

/**
  * @brief  Reads a byte from a specific Camera register 
  * @param  Device: write address.
  * @param  Addr:  register address. 
  * @retval data read from the specific register or 0xFF if timeout condition 
  *         occured. 
  */
uint8_t SCCB_Read16(u16 Reg, uint8_t *Data)
{
  uint32_t timeout = DCMI_TIMEOUT_MAX;
	u32 i;
    /* Clear AF flag if arised */
  I2C2->SR1 |= (uint16_t)0x0400;

  /* Generate the Start Condition */
  I2C_GenerateSTART(VISION_SCCB, ENABLE);

  /* Test on I2C2 EV5 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_MODE_SELECT))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  } 
	
  /*-----------------------------------------------------------------------------------*/
  /* Send DCMI selcted device slave Address for write */
  I2C_Send7bitAddress(VISION_SCCB, sccbadd | 0x01 , I2C_Direction_Transmitter);
 
  /* Test on I2C2 EV6 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  } 
  /*-----------------------------------------------------------------------------------*/
  /* Send DCMI selcted device slave Address for write */
  /* Send I2C2 location address LSB */
  I2C_SendData(VISION_SCCB, (uint8_t)(Reg>>8));
  for(i = 0; i< 0xffff;i++);
	
  /* Test on I2C2 EV6 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  } 
  /*-----------------------------------------------------------------------------------*/
	
  /* Send I2C2 location address LSB */
  I2C_SendData(VISION_SCCB, (uint8_t)(Reg & 0x00FF));
  for(i = 0; i< 0xffff;i++);
  /* Test on I2C2 EV8 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  } 
  
  /* Clear AF flag if arised */
  I2C2->SR1 |= (uint16_t)0x0400;
  /*-----------------------------------------------------------------------------------*/    
  /* Prepare Stop after receiving data */
  I2C_GenerateSTOP(VISION_SCCB, ENABLE);

  /*-----------------------------------------------------------------------------------*/
  /* Generate the Start Condition */
  I2C_GenerateSTART(VISION_SCCB, ENABLE);
  
  /* Test on I2C2 EV6 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_MODE_SELECT))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  } 
  /*-----------------------------------------------------------------------------------*/
  /* Send DCMI selcted device slave Address for write */
  I2C_Send7bitAddress(VISION_SCCB, sccbadd | 0x01, I2C_Direction_Receiver);
  for(i = 0; i< 0xffff;i++);
  /* Test on I2C2 EV6 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  }
      
  /*-----------------------------------------------------------------------------------*/
  /* Prepare an NACK for the next data received */
  I2C_AcknowledgeConfig(VISION_SCCB, DISABLE);

  /* Test on VISION_SCCB EV7 and clear it */
  timeout = DCMI_TIMEOUT_MAX; /* Initialize timeout value */
  while(!I2C_CheckEvent(VISION_SCCB, I2C_EVENT_MASTER_BYTE_RECEIVED))
  {
    /* If the timeout delay is exeeded, exit with error code */
    if ((timeout--) == 0) return 0xFF;
  } 
  /*-----------------------------------------------------------------------------------*/    
  /* Prepare Stop after receiving data */
  I2C_GenerateSTOP(VISION_SCCB, ENABLE); 

  /*-----------------------------------------------------------------------------------*/
  /* Receive the Data */
  *Data = I2C_ReceiveData(VISION_SCCB);
  /* Clear AF flag if arised */
  I2C2->SR1 |= (uint16_t)0x0400;

  /* return the read data */
  return 0;
}