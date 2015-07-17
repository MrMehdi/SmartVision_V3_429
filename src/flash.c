#include "flash.h"
#include "stm32f4xx.h"
#include <stdio.h>
#include "serialcom.h"
#include "camdrv.h"

#define BINARY 		     1
#define NON_BINARY !BINARY

extern void Delayms(unsigned iv);
u8 Flash_Spi_TxRx(u8 data);

void Flash_chip_erase(void);

u8 fbuf[WORKING_PAGE_SZ+16];

void Flash_enb_sect_prot(void);
void Flash_dis_sect_prot(void);

fs file_sys;
u32 Flash_Create_File(void);

u32 FlashFileBytesWritten=0;

u32 const ftb[FTB_FILES_TOTAL]=
{
		BYTES_PER_SECTOR*8,
		BYTES_PER_SECTOR*10,
		BYTES_PER_SECTOR*12,
		BYTES_PER_SECTOR*14,
};

/* 3 address bytes 24 bits
//19 byte address
Page address 					12 bits = 2^12 = 4096 pages
Buffer address bits		9 bits	= 512 (264 byte access required)
Dummy Bits						5 Dummy Bits
*/

u32 Flash_convert_addr(u32 addr, u32 page_size)
{
	u32 at45db_addr=0;
	u32 page_address 	 = addr / WORKING_PAGE_SZ;
	u32 buffer_address = addr % WORKING_PAGE_SZ;
	
	if( WORKING_PAGE_SZ == 528)
		at45db_addr = (page_address << 10) | buffer_address;
	else if( WORKING_PAGE_SZ == 512)
		at45db_addr = (page_address << 9) | buffer_address;
	else
		SerComSendMessageUser("Invalid Flash Address\r\n");

	return at45db_addr;
}

//Return Page address
u32 Flash_get_page_addr(u32 pageno)
{
	u32 at45db_addr=0;
	
	if( WORKING_PAGE_SZ == 512)
		at45db_addr = (pageno << 9);
	else if( WORKING_PAGE_SZ == 528)
		at45db_addr = (pageno << 10);
	else
		SerComSendMessageUser("Invalid Flash Page Acess\r\n");
	
	return at45db_addr;
}

//Check if Flash is Busy
void Flash_wait_busy(void)
{
	u8 v = 0;
	u32 i= 0;
	
	CS_LOW();
	while(i<FLASH_TIMEOUT){
		
		Flash_Spi_TxRx(FLASH_OP_RD_STATUS);
		v = Flash_Spi_TxRx(DUMMY_BYTE);
		if (v != 0xFF)
			if ((v & 0x80) == 0x80){
				break;
			}
		i++;
	}
	if(i>=FLASH_TIMEOUT)
	{
		SerComSendMessageUser("Flash operation timed out\r\n");
	}
	CS_HIGH();
}

u8 Flash_write_buffer(u8* b, u32 add)
{
	u16 i=0;
	CS_LOW();
	Flash_Spi_TxRx(FLASH_OP_BUFFER_WR);
	Flash_Spi_TxRx((add >> 16)&0xff);
	Flash_Spi_TxRx((add >> 8)&0xff);
	Flash_Spi_TxRx((add >> 0)&0xff);
	while(i<WORKING_PAGE_SZ){
		Flash_Spi_TxRx(b[i++]);		
	}
	CS_HIGH();
	Flash_wait_busy();
	return 1;
}

u8 Flash_read_buffer(u8* b, u32 add)
{
	u16 i=0;
	CS_LOW();
	Flash_Spi_TxRx(FLASH_OP_BUFFER_RD);
	Flash_Spi_TxRx((add >> 16)&0xff);
	Flash_Spi_TxRx((add >> 8)&0xff);
	Flash_Spi_TxRx((add >> 0)&0xff);
	Flash_Spi_TxRx(DUMMY_BYTE);	
	while(i<WORKING_PAGE_SZ){
		b[i++] = Flash_Spi_TxRx(DUMMY_BYTE);		
	}
	CS_HIGH();
	Flash_wait_busy();
	
	return 1;
}
u8 Flash_read_buffer_flash(u32 add)
{
	CS_LOW();
	Flash_Spi_TxRx(FLASH_OP_BF_LOAD_FLASH);
	Flash_Spi_TxRx((add >> 16)&0xff);
	Flash_Spi_TxRx((add >> 8)&0xff);
	Flash_Spi_TxRx((add >> 0)&0xff);
	CS_HIGH();
	Flash_wait_busy();
	return 1;
}
u8 Flash_write_buffer_flash(u32 add)
{
	CS_LOW();
	Flash_Spi_TxRx(FLASH_OP_BF_WRITE_FLASH);
	Flash_Spi_TxRx((add >> 16)&0xff);
	Flash_Spi_TxRx((add >> 8)&0xff);
	Flash_Spi_TxRx((add >> 0)&0xff);
	CS_HIGH();
	Flash_wait_busy();
	return 1;
}
u8 Flash_read_arr(u32 add, u8* b,u32 n)
{
	u32 i=0;
	CS_LOW();
	Flash_Spi_TxRx(FLASH_OP_ARR_READ_HF);
	Flash_Spi_TxRx((add >> 16)&0xff);
	Flash_Spi_TxRx((add >> 8)&0xff);
	Flash_Spi_TxRx((add >> 0)&0xff);
	Flash_Spi_TxRx(DUMMY_BYTE);
	while(n>0){
		b[i++] = Flash_Spi_TxRx(DUMMY_BYTE);
		n--;
	}
	CS_HIGH();
	Flash_wait_busy();
	
	return 1;
}

u8 Flash_wr_mem_and_buffer(u8* b, u32 add)
{
	u16 i=0;
	CS_LOW();
	Flash_Spi_TxRx(FLASH_OP_BUFFER_WR_MEM);
	Flash_Spi_TxRx(add >> 16);
	Flash_Spi_TxRx(add >> 8);
	Flash_Spi_TxRx(add >> 0);
	while(i<WORKING_PAGE_SZ){
		 Flash_Spi_TxRx(b[i++]);		
	}
	
	CS_HIGH();
	Flash_wait_busy();
	
	return 1;
}

u8 Flash_wr_mem(u8* b, u32 add,u16 len)
{
	u16 i=0;
	
#ifdef DBG_FLASH
	u8 sb[50];
	sprintf(sb,"FL A:%x,L:%x\r\n",add,len);
	SerComSendMessageUser(sb);
#endif
	CS_LOW();
//	SerComSendMessageUser("---DATA---");
	Flash_Spi_TxRx(FLASH_OP_WR_MEM);
	Flash_Spi_TxRx(add >> 16);
	Flash_Spi_TxRx(add >> 8);
	Flash_Spi_TxRx(add >> 0);
	while(i<len){
		 Flash_Spi_TxRx(b[i++]);
//		 SerComSendArrayUser(b+i,1);		
	}
//	SerComSendMessageUser("---XXX---");
#ifdef DBG_FLASH
	//
	//SerComSendArrayUser(b,len);
	//
#endif
	
	CS_HIGH();
	Flash_wait_busy();
	
	return 1;
}

// // u8 Flash_Write_Memory_Array(u32 add , u8 * b, u32 n)
// // {
// // 	unsigned i=0;
// // 	
// // 	while(i < n)
// // 	{
// // 		Flash_wr_mem(&b[i],Flash_convert_addr(add + i,WORKING_PAGE_SZ));
// // 		i += WORKING_PAGE_SZ;
// // 	}
// // 	
// // 	return 1;
//}
u8 Flash_Sector_Erase(u32 add)
{
	CS_LOW();
	Flash_Spi_TxRx(FLASH_SECTOR_ERASE);
	Flash_Spi_TxRx(add >> 16);
	Flash_Spi_TxRx(add >> 8);
	Flash_Spi_TxRx(add >> 0);
	CS_HIGH();
	Flash_wait_busy();
	
	return 1;
}

u8 Flash_Configure_Pagesz(u32 pagesz)
{
	if(pagesz == BINARY)
	{
		CS_LOW();
		Flash_Spi_TxRx(0x3d);
		Flash_Spi_TxRx(0x2a);
		Flash_Spi_TxRx(0x80);
		Flash_Spi_TxRx(0xa6);
		CS_HIGH();
		Flash_wait_busy();
	}
	else if(pagesz == NON_BINARY)
	{
		CS_LOW();
		Flash_Spi_TxRx(0x3d);
		Flash_Spi_TxRx(0x2a);
		Flash_Spi_TxRx(0x80);
		Flash_Spi_TxRx(0xa7);
		CS_HIGH();
		Flash_wait_busy();
	}
	else
	{
			return 0;
	}
	
	return 1;
}
/*
u8 Flash_write_sram(u32 sa, u32 fa , u32 n)
{
	u32 i=0,sz;
	while(i < n)
	{
		sz = n-i;
		if(sz > WORKING_PAGE_SZ)
			sz = WORKING_PAGE_SZ;
		else
			sz= sz;
		
			if( sz % 2)sz++;
			SRAM_ReadBuffer((u16*)fbuf,sa,sz/2); //read data from SRAM into RAM
			Flash_wr_mem(fbuf,Flash_convert_addr(fa + i,WORKING_PAGE_SZ),sz);//Write data to Flash
			i  += sz;
			sa += sz;
	}
	return 1;
}
*/


/*
u32 Flash_Write_File(u32 add, u8 n, u32 max_sz)
{
	u8 i = 1;
	s32 add1, add2;
	u32 sz,ts = 0;
	add1 = 0;

	while(i <= n)
	{
		add1 = SRAM_Search16(0xFFD8,add1,max_sz);//Search Header
		
		add2 = SRAM_Search16(0xFFD9,add1,max_sz);//Search Footer
		


		if ( add2 > add1)
		{
			add2 += 2;
			SerComSendMessageUser("Writing To Flash Image Size ");
			sz = add2-add1;
			imgdebug.filesz[i-1] = sz;
			sprintf(sb,"%d: %d\r\n",i,sz);
			SerComSendMessageUser(sb);
			sprintf(sb,"%d:%d:%d\r\n",add1 ,add + ts,sz);
			SerComSendMessageUser(sb);
			Flash_write_sram(add1 ,add + ts,sz); // Store Image 1
			add1 = add2;
			ts += sz;
			if((ts%WORKING_PAGE_SZ) > 0)
			{
				ts += WORKING_PAGE_SZ-(ts%WORKING_PAGE_SZ);
			}
			i++;
		}
		else
		{
			imgdebug.error |= (1<< (IMGERR_NFOUND0+i));

		}
				

		
	}
	
 SerComSendMessageUser("Total Written:");	
 sprintf(sb," %d\r\n",ts);
 SerComSendMessageUser(sb);
	
 return ts;
}
*/
//u32 GetJpegImgSize(u8 *b , u32 szmax)
//{
//	u32 i,sz = 0;
//	
//	//Get size of image
//	if (b[0] == 0xFF && b[1] == 0xD8)
//	{
//		for(i=2 ; i < szmax; i ++)
//		{
//			if(JpegBuffer[i] == 0xFF && JpegBuffer[i+1] == 0xD9)
//			{
//				sz = i+1;
//				break;
//			}
//		}
//	}
//	return sz;
//}
/*
u32 Flash_store_image_data(u32* nseq)//returns size of stored image(both images together) and sequence number
{
		
	u32 t,i,sz1=0,sz2=0;
	u8 b1fail=0;
	fe * fileinfo;
	u32 lfno;
	u32 lseqno=0;
	u32 hseqno=0;

	s32 add1, add2;
	
	imgdebug.header= 0XABCDEF01;
	imgdebug.captime = jpegTotalTime;
	imgdebug.error = 0;
	imgdebug.filesz[0]=0;
	imgdebug.filesz[1]=0;
	imgdebug.seqno=0;
	imgdebug.footer = 0xFFFE;
	
	sz1 = GetJpegImgSize(JpegBuffer,JPEG_BUFFER_SZ);
	
	if(sz == 0)//Image was not found check the offload buffers
	{
		memcpy((void*)JpegBuffer,Jpeg_b1,OFFLOAD_BUFFER_1SZ);
		memcpy((void*)&JpegBuffer[OFFLOAD_BUFFER_1SZ],Jpeg_b2,OFFLOAD_BUFFER_2SZ);
		sz = GetJpegImgSize(JpegBuffer,JPEG_BUFFER_SZ);
	}
		
	if( sz > 0 )//
	{
		Flash_dis_sect_prot();//Disable Flash Protection
	
		Flash_Read(FTABLE_ADDRESS,(void*)&file_sys,sizeof(file_sys));//Read File System
	
		if ( file_sys.header  != 0x01ABCDEF)
		{
			SerComSendMessageUser("File System Corrupt\r\n");
			Flash_Create_FS();
			return ftb[0];
		}
	
		//Erase page store image
	}
	
	//Get size of image
	if(Jpeg_b1[0] == 0xFF && Jpeg_b1[1] == 0xD8)
	{
		memcpy((void*)JpegBuffer,Jpeg_b1,OFFLOAD_BUFFER_1SZ);
		memcpy((void*)&JpegBuffer[OFFLOAD_BUFFER_1SZ],Jpeg_b2,OFFLOAD_BUFFER_2SZ);
		FlashProcessStoreJpeg();
	}
	
}
*/
u8 FlashCreateNewFile(u32* seq)
{
	u8 sb[50];
	u8 i=0;
	u32 t;
	fe * fileinfo;
	u32 lfno;
	u32 lseqno=0;
	u32 hseqno=0;

	s32 add1, add2;
	
	Flash_Read(FTABLE_ADDRESS,(void*)&file_sys,sizeof(file_sys));//Read File System
	
	if ( file_sys.header  != 0x01ABCDEF)
	{
		SerComSendMessageUser("File System Corrupt, Creating New\r\n");
		Flash_Create_FS();
		*seq = 1;
		return 0;
	}
	//Search for a file to write to
	fileinfo = (fe *)&file_sys.ftable[0];
	lfno = 0;
	lseqno = fileinfo->seq;
	hseqno = lseqno;
	
	for(i = 1; i<FTB_FILES_TOTAL;i++)
	{
		fileinfo = (fe *)&file_sys.ftable[i*4];
		
		if( lseqno > fileinfo->seq)
		{
			lseqno = fileinfo->seq;	//Store lowest seqno
			lfno   = i;				//Store file no for the lowest seqno
		}
		if(fileinfo->seq > hseqno)
		{
			hseqno = fileinfo->seq; //Store highest seqno
		}
	}
	fileinfo = (fe *)&file_sys.ftable[lfno];
	SerComSendMessageUser("NEW FILE LOCATION:");
	sprintf(sb," %d ",lfno);
	SerComSendMessageUser(sb);
	SerComSendMessageUser("NEW SEQNO:");
	sprintf(sb," %d ",hseqno+1);
	SerComSendMessageUser(sb);
	
	*seq = hseqno + 1;
	
	SerComSendMessageUser("Erasing Sector Address:");
	t = Flash_convert_addr(fileinfo->sof + 0*(BYTES_PER_SECTOR),512);
	sprintf(sb," %d\r\n",t);
	SerComSendMessageUser(sb);
	Flash_Sector_Erase(t);
	SerComSendMessageUser("Erasing Sector Address:");
	t = Flash_convert_addr(fileinfo->sof + 1*(BYTES_PER_SECTOR),512);
	sprintf(sb," %d\r\n",t);
	SerComSendMessageUser(sb);
	Flash_Sector_Erase(t);
	FlashFileBytesWritten = 0;
	return lfno;//return file handle

}
u8 FlashCloseFile(u8 handle, u32 seq)//have to close file on open to update file table
{
	fe * fileinfo;	
	Flash_Read(FTABLE_ADDRESS,(void*)&file_sys,sizeof(file_sys));//Read File System
	fileinfo = (fe *)&file_sys.ftable[handle*4];
	file_sys.header = 0x01ABCDEF;
	fileinfo->seq = seq;
	fileinfo->sz =  FlashFileBytesWritten;
	fileinfo->file_tag  = 'D';   //Mark as new file
	fileinfo->sof = ftb[handle]; //Update Start file address in flash from lookup tble
	Flash_Write(FTABLE_ADDRESS,(void*)&file_sys,sizeof(file_sys)); //write information to flash
;
	return 0;
}

u32 FlashFileWrite(u32 offset,u8 handle, u8*data, u32 n)
{
	u32 i=0,sz;
	u32 fa;
	
	fa = ftb[handle];//Get the start of flash address to write from the lookup table
	
	while(i < n)
	{
		sz = n-i;
		if(sz > WORKING_PAGE_SZ)
			sz = WORKING_PAGE_SZ; //write pagewise ie 512 bytes at a time to avoid overwrite
		else
			sz= sz;
		

		
    //Flash_Write(Flash_convert_addr(fa + i + offset,WORKING_PAGE_SZ),data+i,sz);
		Flash_wr_mem(data+i,Flash_convert_addr(fa + i + offset,WORKING_PAGE_SZ),sz);//Write data to Flash
		i  += sz;			
	}
	return n;//return total bytes written,in this case always equal to the bytes to be written passed as an argument
}	


u8 Flash_Write(u32 fa, u8 * arr , u32 len)
{
	u8 i=0;
	u32 add;
	if(len > WORKING_PAGE_SZ)
	{
		SerComSendMessageUser("Page Fault Exiting Write\r\n");
		return 0;
	}
	Flash_dis_sect_prot();
	CS_LOW();
	
	add = Flash_convert_addr(fa,WORKING_PAGE_SZ);
	
	Flash_Spi_TxRx(FLASH_OP_BUFFER_WR_MEM);
	Flash_Spi_TxRx(add >> 16);
	Flash_Spi_TxRx(add >> 8);
	Flash_Spi_TxRx(add >> 0);
	
	while(len){
		Flash_Spi_TxRx(arr[i++]);	
		len--;
	}

	CS_HIGH();
	Flash_wait_busy();
	
	return 1;

}

u8 Flash_Read(u32 fa,u8 * arr , u32 len)
{
	u32 i=0;
	u32 add = Flash_convert_addr(fa,WORKING_PAGE_SZ);
	CS_LOW();
	Flash_Spi_TxRx(FLASH_OP_ARR_READ_HF);
	Flash_Spi_TxRx((add >> 16)&0xff);
	Flash_Spi_TxRx((add >> 8)&0xff);
	Flash_Spi_TxRx((add >> 0)&0xff);
	Flash_Spi_TxRx(DUMMY_BYTE);
	while(len > 0){
		arr[i++] = Flash_Spi_TxRx(DUMMY_BYTE);
		len --;
	}
	CS_HIGH();
	Flash_wait_busy();
	return 0;
}
void Flash_Dump_filetable(void)
{
	u8 i;
	fe * fileinfo;
	u8 sb[50];
	
	Flash_Read(FTABLE_ADDRESS,(void*)&file_sys,sizeof(file_sys));

	SerComSendMessageUser("HEADER:");
	sprintf(sb," %X ",file_sys.header);
	SerComSendMessageUser(sb);
	
	SerComSendMessageUser("MAX_FILES:");
	sprintf(sb," %d ",file_sys.max_fno);
	SerComSendMessageUser(sb);
	
	SerComSendMessageUser("MAX_FILE_SIZE:");
	sprintf(sb," %d ",file_sys.max_sz);
	SerComSendMessageUser(sb);
	
	SerComSendMessageUser("\r\nFILE TABLE:\r\n");
	for(i = 0; i<FTB_FILES_TOTAL;i++)
	{
		fileinfo = (fe *)&file_sys.ftable[i*4];
			
		SerComSendMessageUser("FILE TAG:");
		sprintf(sb," %x ",fileinfo->file_tag);
		SerComSendMessageUser(sb);
		
		SerComSendMessageUser("FILE SEQ:");
		sprintf(sb," %d ",fileinfo->seq);
		SerComSendMessageUser(sb);
		
		SerComSendMessageUser("FILE ADDRESS:");
		sprintf(sb," %d ",fileinfo->sof);
		SerComSendMessageUser(sb);
		
		SerComSendMessageUser("FILE SIZE:");
		sprintf(sb," %d ",fileinfo->sz);
		SerComSendMessageUser(sb);
		
		SerComSendMessageUser("\r\n");
	}
			SerComSendMessageUser("SR #:");
	         for( i=0;i<18;i++)
			{
				sprintf(sb," %x ",imgdebug.device_serial[i]);
				SerComSendMessageUser(sb);
			}
	
	SerComSendMessageUser("\r\n");	
}

void Flash_Create_FS(void)
{
	u8 i;
	fe * fileinfo;
	
	SerComSendMessageUser("Creating New Tables\r\n");
	
	file_sys.header = 0x01ABCDEF;
	file_sys.max_fno = FTB_FILES_TOTAL;
	file_sys.max_sz = MAX_FILE_ENTRY;
	
	for(i = 0; i<FTB_FILES_TOTAL;i++)
	{
		fileinfo = (fe *)&file_sys.ftable[i*4];
		fileinfo->file_tag = 'E';
		fileinfo->seq = 0;
		fileinfo->sof = ftb[i];
		fileinfo->sz = 0;
	}
	SerComSendMessageUser("Writing to Flash\r\n");
	Flash_Write(FTABLE_ADDRESS,(void*)&file_sys,sizeof(file_sys));
}

u32 Flash_get_file_size(u8 fno)
{
	fe * fileinfo;
	u32 sz;
	
	Flash_Read(FTABLE_ADDRESS,(void*)&file_sys,sizeof(file_sys));
	
	if ( file_sys.header  != 0x01ABCDEF)
	{
		SerComSendMessageUser("File System Corrupt\r\n");
		Flash_Create_FS();
		return 0;
	}
	fileinfo =(fe*)&file_sys.ftable[fno*4];//Read file info data into structure pointer
	if(fileinfo->sz > MAX_FILE_ENTRY)//Invalid File size
	{
		SerComSendMessageUser("File Info Corrupt\r\n");
		Flash_Create_FS();
		return 0;
	}
	return fileinfo->sz;
}

void Flash_dump_imagedata(u8 fno)
{
	u32 sz,add;
	
	sz  = Flash_get_file_size(fno);
	add = Flash_convert_addr(ftb[fno],WORKING_PAGE_SZ);

	CS_LOW();
	Flash_Spi_TxRx(FLASH_OP_ARR_READ_HF);
	Flash_Spi_TxRx((add >> 16)&0xff);
	Flash_Spi_TxRx((add >> 8) &0xff);
	Flash_Spi_TxRx((add >> 0) &0xff);
	Flash_Spi_TxRx(DUMMY_BYTE);
	while(sz > 0){
		//SerComSendByte(Flash_Spi_TxRx(DUMMY_BYTE));
		while (USART_GetFlagStatus(UART7, USART_FLAG_TC) == 0);
		USART_SendData(UART7,Flash_Spi_TxRx(DUMMY_BYTE));
		while (USART_GetFlagStatus(UART7, USART_FLAG_TC) == 0);
		sz--;
	}
	CS_HIGH();
	Flash_wait_busy();
	
}



void Flash_soft_reset(void)
{
	CS_LOW();
	Flash_Spi_TxRx(0xF0);
	Flash_Spi_TxRx(0);
	Flash_Spi_TxRx(0);
	Flash_Spi_TxRx(0);
	CS_HIGH();
	Flash_wait_busy();
}
void Flash_enb_sect_prot(void)
{
	CS_LOW();
	Flash_Spi_TxRx(0x3d);
	Flash_Spi_TxRx(0x2a);
	Flash_Spi_TxRx(0x7f);
	Flash_Spi_TxRx(0xa9);
	CS_HIGH();
	Flash_wait_busy();
}
void Flash_dis_sect_prot(void)
{
	CS_LOW();
	Flash_Spi_TxRx(0x3d);
	Flash_Spi_TxRx(0x2a);
	Flash_Spi_TxRx(0x7f);
	Flash_Spi_TxRx(0x9a);
	CS_HIGH();
	Flash_wait_busy();
}
void Flash_chip_erase(void)
{
	CS_LOW();
	Flash_Spi_TxRx(0xc7);
	Flash_Spi_TxRx(0x94);
	Flash_Spi_TxRx(0x80);
	Flash_Spi_TxRx(0x9a);
	CS_HIGH();
	Flash_wait_busy();
}
u32 Flash_read_prot_reg(void)
{
	u32 prt = 0;
	CS_LOW();
	Flash_Spi_TxRx(0x32);
	Flash_Spi_TxRx(DUMMY_BYTE);
	Flash_Spi_TxRx(DUMMY_BYTE);
	Flash_Spi_TxRx(DUMMY_BYTE);
	prt |= Flash_Spi_TxRx(DUMMY_BYTE);
	prt<<=8;
	prt |= Flash_Spi_TxRx(DUMMY_BYTE);
	prt<<=8;
	prt |= Flash_Spi_TxRx(DUMMY_BYTE);
	CS_HIGH();
	Flash_wait_busy();
		
	return prt;
}	

u16 Flash_read_status(void)
{
	u16 v = 0;
	
	CS_LOW();
	Flash_Spi_TxRx(FLASH_OP_RD_STATUS);
	v |= Flash_Spi_TxRx(DUMMY_BYTE);
	v<<=8;
	v |= Flash_Spi_TxRx(DUMMY_BYTE);
	Delayms(CS_LOW_DLY_MS);
	CS_HIGH();
	Flash_wait_busy();
	return v;
}	

u32 Flash_read_ID(void)
{
	 u32 id=0;

	CS_LOW();
	Flash_Spi_TxRx(FLASH_OP_RD_ID);
	id |= Flash_Spi_TxRx(DUMMY_BYTE);
	id<<=8;
	id |= Flash_Spi_TxRx(DUMMY_BYTE);
	id<<=8;
	id |= Flash_Spi_TxRx(DUMMY_BYTE);
	id<<=8;
	id |= Flash_Spi_TxRx(DUMMY_BYTE);
	CS_HIGH();
	Flash_wait_busy();
		
	return id;
}
u8 Flash_Spi_TxRx(u8 data){

	SPI4->DR = data; 			//transmit
	while( !(SPI4->SR & SPI_I2S_FLAG_TXE) ); 	// wait until transmit complete
	while( !(SPI4->SR & SPI_I2S_FLAG_RXNE) ); // wait until receive complete
	while( SPI4->SR & SPI_I2S_FLAG_BSY ); // wait until SPI is not busy anymore
	return SPI4->DR; // recieve
}

u8 Flash_init(void)//check if flash is OK and update parameters
{
  u16 v;
  u8 sb[50];
  Flash_dis_sect_prot();
	
  Flash_Configure_Pagesz(BINARY);
	
	v = Flash_read_status();
	
	//1011 - 16 Mbit Version
	//1101 - 32 Mbit Version
	//0101 - 2 Mbit Version
	
	SerComSendMessageUser("Working Page Size:");
	sprintf(sb," %d Bytes\r\n",WORKING_PAGE_SZ);
	SerComSendMessageUser(sb);
	
	if((v & 0x0100))
	{
		SerComSendMessageUser("Binary Page Size\r\n");
	}
	else
	{
		SerComSendMessageUser("Unsupported Flash!\r\n");
		return 0;
	}
	if( (v & (0x0f << 10)) == (5 << 10))
	{
		SerComSendMessageUser("2-Mbit Flash Detected\r\n");
		SerComSendMessageUser("Unsupported Flash!\r\n");
		return 0;
	}
	if( (v & (0x0f << 10)) == (0x0b << 10))
	{
		SerComSendMessageUser("16-Mbit Flash Detected\r\n");
#ifndef M16
		SerComSendMessageUser("Unsupported Flash!\r\n");
		return 1;
#endif
	}
	if( (v & (0x0f << 10)) == (0x0d << 10))
	{
		SerComSendMessageUser("32-Mbit Flash Detected\r\n");
#ifndef M32
		SerComSendMessageUser("Unsupported Flash!\r\n");
		return 1;
#endif
	}
	SerComSendMessageUser("Flash Init Done!");
	return 0;
}

u8 Flash_Init_SPI(void){
	
	GPIO_InitTypeDef GPIO_InitStruct;
	SPI_InitTypeDef SPI_InitStruct;
	
	// enable clock for used IO pins
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	
	/* configure pins used by SPI1
	 * PE2 = SCK
	 * PE5 = MISO
	 * PE6 = MOSI
	 */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_5 | GPIO_Pin_6;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;//GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOE, &GPIO_InitStruct);
	
	// connect SPI1 pins to SPI alternate function
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource2, GPIO_AF_SPI4);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource5, GPIO_AF_SPI4);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource6, GPIO_AF_SPI4);
	
	// enable clock for used IO pins
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	
	/* Configure the chip select pin
	   in this case we will use PE4 */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOE, &GPIO_InitStruct);
	
	GPIOB->BSRRL |= GPIO_Pin_4; // set PE7 high
	
	// enable peripheral clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI4, ENABLE);
	
	/* configure SPI1 in Mode 0 
	 * CPOL = 0 --> clock is low when idle
	 * CPHA = 0 --> data is sampled at the first edge
	 */
	SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex; // set to full duplex mode, seperate MOSI and MISO lines
	SPI_InitStruct.SPI_Mode = SPI_Mode_Master;     // transmit in master mode, NSS pin has to be always high
	SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b; // one packet of data is 8 bits wide
	SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;        // clock is low when idle
	SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;      // data sampled at first edge
//	SPI_InitStruct.SPI_NSS = SPI_NSS_Soft | SPI_NSSInternalSoft_Set; // set the NSS management to internal and pull internal NSS high
	SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256; // SPI frequency is APB2 frequency / 4
	SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;// data is transmitted MSB first
	SPI_Init(SPI4, &SPI_InitStruct); 
	
	SPI_Cmd(SPI4, ENABLE); // enable SPI1
	Delayms(50);
		
	return Flash_init();
	
}

//OLD Code----
// u8 Flash_test(void)
// {
// 	unsigned i;
// 	
// 	for( i =0; i< MAX_IMAG_SZ/2; i++)
// 	{
// 		fbuf[i]= (i & 0xff);
// 	}
// 	
// 	//Flash_chip_erase();
// 	SRAM_WriteBuffer((u16*)fbuf,0,MAX_IMAG_SZ/4);
// 	SRAM_WriteBuffer((u16*)fbuf,MAX_IMAG_SZ/2,MAX_IMAG_SZ/4);
// 	Flash_write_sram(0,FLASH_IMAGE_START_ADDRESS,MAX_IMAG_SZ);
// 	Flash_dump_imagedata(Flash_convert_addr(FLASH_IMAGE_START_ADDRESS,264),MAX_IMAG_SZ);
// 	//SRAM_dump_imagedata();
// 	
// 	
// 	//SRAM_WriteBuffer((u16*)tbuf,0,TEST_SZ/2);
// 	//st = systime;
// 	//Flash_write_sram(0,FLASH_IMAGE_START_ADD,TEST_SZ);
// 	//st = systime-st;
// 	while(1);
// 	memset(fbuf,0,KB100);
// 	Flash_read_arr(Flash_convert_addr(0,page_sz),fbuf,KB100);
// 	for( i=0; i< KB100; i++)
// 	{
// 		if (fbuf[i] != (i & 0xff))
// 		{
// 				while(1);
// 		}
// 	}
// 	memset(fbuf,0,KB100);
// 	Flash_read_arr(Flash_convert_addr(KB100,page_sz),fbuf,KB100);
// 	for( i=0; i< KB100; i++)
// 	{
// 		if (fbuf[i] != (i & 0xff))
// 		{
// 				while(1);
// 		}
// 	}
// 	
// 	
// 	
// 	//memset(tbuf,0,30);
// 	//Flash_read_buffer(tbuf,0);
// 	//
//   	Flash_write_buffer(fbuf,0);
//  	  Flash_read_buffer(fbuf,0);
// // 	
//   	Flash_write_buffer_flash(0);
//   	Flash_read_buffer_flash(0);
// // 	Flash_read_buffer(tbuf,0);
// 	
// 	while(1)
// 	{
// 		Flash_wait_busy();
// 	}
// }