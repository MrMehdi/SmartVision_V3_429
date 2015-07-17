#ifndef __FLASH_H_
#define __FLASH_H_
#include "stm32f4xx.h"
#include "camdrv.h"
	#define WORKING_PAGE_SZ  512			//page size config
	
	#define FTABLE_ADDRESS 				  0xFF200
	#define FLASH_CAM_SETTING_ADD		0xFF000
	#define FLASH_CAM_SERIAL_ADD		0xFEE52
	#define MAX_FILE_ENTRY 				  256*1024
	#define FTB_FILES_TOTAL         4
	
	u8 Flash_Init_SPI(void);
	
	#define FLASH_OP_ARR_READ_LEG 0xE8 //NRND
	#define FLASH_OP_ARR_READ_HF 	0x0b
	#define FLASH_OP_ARR_READ_LF 	0x03
	#define FLASH_OP_ARR_READ_LP	0x01
	#define FLASH_OP_BUFFER_RD		0xD4 //ANY F
	#define FLASH_OP_BUFFER_WR		0x84	
	#define FLASH_OP_BF_PG_WR			0x83
	#define FLASH_OP_BUFFER_WR_MEM 0x82
	#define FLASH_OP_WR_MEM 			0x02
	#define FLASH_SECTOR_ERASE		0x7c
	#define FLASH_OP_RD_ID				0x9f
	#define FLASH_OP_RD_STATUS		0xD7
	#define FLASH_OP_BF_LOAD_FLASH 0x53
	#define FLASH_OP_BF_WRITE_FLASH 0x83
	#define CS_LOW_DLY_MS					1
	#define CS_LOW_DLY_MS_WR			1000

	#define CS_LOW()				GPIO_ResetBits(GPIOE, GPIO_Pin_4)
	#define CS_HIGH()				GPIO_SetBits(GPIOE, GPIO_Pin_4);

	#define FLASH_TIMEOUT					0xFFFFFF 
	#define DUMMY_BYTE						0xaa
	
	#define IMGERR_NFOUND0				0
	#define IMGERR_NFOUND1				1

	//16-Mbit Flash is a total of 4095 pages of 512 bytes =  2096640 Bytes
  //Image region 2048 pages 1048596(1MByte) 4 images = 256Kb per image
	//128KB image erase
	#define FLASH_MAX_IMAGES 4
  #define SECTOR_PER_IMAGE 2
	
	#define BYTES_PER_PAGE WORKING_PAGE_SZ
	#define PAGES_PER_BLOCK 8
	#define BLOCKS_PER_SECTOR 32
	#define BYTES_PER_SECTOR BYTES_PER_PAGE*PAGES_PER_BLOCK*BLOCKS_PER_SECTOR
	
	u8 Flash_Read(u32 fa,u8 * arr , u32 len);
	
	u8 Flash_Write(u32 fa,u8 * arr , u32 len);

	u32 Flash_Store_JPEG_SRAM(u8 n, u32 * nseq);

	
	typedef struct
	{
		u32 file_tag;
		u32 sof;
		u32 sz;
		u32 seq;
	}fe;
		
	typedef struct
	{
		u32 header;
		u32 max_sz;
		u32 max_fno;
		u32 dummy;
		u32 ftable[FTB_FILES_TOTAL*4];
	}fs;
	

	
	extern fs file_sys;
	void Flash_Create_FS(void);
	void Flash_Dump_filetable(void);
	u8 FlashCreateNewFile(u32* seq);
	u8 FlashCloseFile(u8 handle, u32 seq);
	u32 FlashFileWrite(u32 offset,u8 handle, u8*data, u32 n);
	
#endif
