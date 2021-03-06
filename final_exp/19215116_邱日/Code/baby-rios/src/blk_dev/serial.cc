#include <rios/serial.h>
#include <rios/console.h>
#include <rios/gdt_idt.h>
unsigned char *buffer_addr=(unsigned char *)0x888888;/*mm 0x666666*/
int disk_no =0 ;/*disk0 is your Udisk ,and disk1 is hdb(hard disk on your PC)*/
/* @param nr_disk can only be 1 or 0.*/
void set_disk_no(int nr_disk)
{
	if(nr_disk ==0){
/*read & write to hda*/
		disk_no =0;
	}else if(nr_disk ==1){
/*read & write to hdb*/
		disk_no =1;
	}
}

void IDE_disk_wait()
{
	
	while ( ( inb_wait(ATA_PORT_STATUS) & (BUSY_STAT|READY_STAT) )!=  READY_STAT )
	    ;
	//print("wait...ok ");
}

void IDE_write_sector(void *src,int lba)
{
	switch_to_disk(1);
	set_disk_no(1);/*disk1 :PC hard disk */
	IDE_disk_wait();
	outb_wait(ATA_PORT_SECT_COUNT,1);	/*outb(0x1f2,1);*/
	outb_wait(ATA_PORT_LBA_LOW ,lba);	/*outb(0x1f3,lba);*/
	outb_wait(ATA_PORT_LBA_MID ,lba >> 8);	/*outb(0x1f4,lba>>8)*/
	outb_wait(ATA_PORT_LBA_HIGH ,lba >> 16);/*outb(0x1f5,lba>>16)*/
	outb_wait(ATA_PORT_DEVICE , 0xe0 |(disk_no&1)<<4| (lba >> 24));/*outb(0x1f5,lba>>16)*/
	/*disk_no determines write to which disk.*/
	outb_wait(ATA_PORT_STATUS, HD_WRITE);
	IDE_disk_wait();
	for(int i = 0; i < SECTOR_SIZE/4 ; i++){
		 _out_data32(ATA_PORT_DATA,((u32*)src)[i]);
	}
}

void IDE_read_sector(void *dest, int lba)
{
	switch_to_disk(1);
	set_disk_no(1);/*disk1 :PC hard disk */
	IDE_disk_wait();
	outb_wait(ATA_PORT_SECT_COUNT,1);	/*outb(0x1f2,1);*/
	outb_wait(ATA_PORT_LBA_LOW ,lba);	/*outb(0x1f3,lba);*/
	outb_wait(ATA_PORT_LBA_MID ,lba >> 8);	/*outb(0x1f4,lba>>8)*/
	outb_wait(ATA_PORT_LBA_HIGH ,lba >> 16);/*outb(0x1f5,lba>>16)*/
	outb_wait(ATA_PORT_DEVICE , 0xe0 |(disk_no&1)<<4 | (lba >> 24));/*outb(0x1f5,lba>>16)*/
	/*disk_no determines read which disk.*/
	outb_wait(ATA_PORT_STATUS, HD_READ );
	IDE_disk_wait();
	for(int i = 0; i < SECTOR_SIZE/4 ; i++){
		 ((u32*)dest)[i]=_in_data32(ATA_PORT_DATA);
	}
}

/*	@param nr_hd can only be 0 or 1
 *      hd0 is Udisk,hd1 or hdb is your PC hard disk
 *	DANGEROUS!!! Please confirm that your PC hard disk 
 *      DONNOT contain important data,or just test in Virtual machine
 */
int hdb_sect_total = 0;
int get_hd_size(int nr_hd)
{
	u8 hd_buf[512*2];
	switch_to_disk(nr_hd);/*hd0 or hd1*/
	int cmd_device = (0xa0 | ((1 & 1) << 4) | ((0 >> 24) & 0x0f)); /*CHS mode*/
	int cmd_command = HD_IDENTIFY;
	_IDE_disk_wait(0);
	outb(ATA_PORT_SECT_COUNT, 0);		/*outb(0x1f2,n_sectors)*/
	outb(ATA_PORT_LBA_LOW, 0 & 0xff);	
	outb(ATA_PORT_LBA_MID, (0 >> 8) & 0xff);
	outb(ATA_PORT_LBA_HIGH, (0 >>16) &0xff);
        outb(ATA_PORT_STATUS, cmd_device);
	outb(ATA_PORT_COMMAND, cmd_command);
	port_read(ATA_PORT_DATA,hd_buf,512);
	//nextline();print("Detecting HD size...");
	u16 * hd_info = (u16*)hd_buf;
	int sectors = ((int)hd_info[61] << 16) +hd_info[60];
	//print("HD size:");
	putnum(sectors*512/1000000);print("MB");
	if(nr_hd == 1) hdb_sect_total = sectors; 
	/*calculate hdb total inodes*/
	return sectors*512/1000000;/*MB*/
}

void msg_get_hda_hdb_info()
{
	nextline();
	msg_ok();
	print("  Detecting HD size... hda = ");get_hd_size(0);
	print(", hdb = ");get_hd_size(1);print(".");
}

void init_hd()
{
	//init_serial();
	judge_disk1_exist();
	msg_get_hda_hdb_info();
	switch_to_disk(1);msg_danger_hdb();
	set_disk_no(1);/*Dangerous!!! read & write to hdb*/
}

int _IDE_disk_wait(int check_error)
{
	int _r;

	while (( (_r= inb_wait(ATA_PORT_STATUS)) & (BUSY_STAT|READY_STAT) ) != READY_STAT)
		; /*judge whether it's busy or ready,wait*/
	if( check_error && (WRERR_STAT|ERR_STAT) != 0)
		return -1;
	return 0;
}

void judge_disk1_exist()
{
	int havedisk1 =0;
 	outb(ATA_PORT_CURRENT,Get_Device_register(1,0));/*outb(0x1f6, 0xe0 | (1<<4));*/
/*switch_to_disk(1)*/
	int i;
	for(i=0; i<1000; i++){
	    if(inb_wait(ATA_PORT_STATUS) != 0){
	      havedisk1 = 1;
	      break;
	    }
	}
	msg_hd1_exist(havedisk1);
	switch_to_disk(0);
	set_disk_no(0);/*switch back to disk 0*/
}
/*about serial ports:https://wiki.osdev.org/Serial_Ports*/

void IDE_write2buf(int lba)
{
	for(int i=0;i<512;i++)_global_buf[i]=0;
	switch_to_disk(1);
	set_disk_no(1);/*disk1 :PC hard disk */
	IDE_disk_wait();
	outb_wait(ATA_PORT_SECT_COUNT,1);	/*outb(0x1f2,1);*/
	outb_wait(ATA_PORT_LBA_LOW ,lba);	/*outb(0x1f3,lba);*/
	outb_wait(ATA_PORT_LBA_MID ,lba >> 8);	/*outb(0x1f4,lba>>8)*/
	outb_wait(ATA_PORT_LBA_HIGH ,lba >> 16);/*outb(0x1f5,lba>>16)*/
	outb_wait(ATA_PORT_DEVICE , 0xe0 |(disk_no&1)<<4| (lba >> 24));/*outb(0x1f5,lba>>16)*/
	/*disk_no determines write to which disk.*/
	outb_wait(ATA_PORT_STATUS, HD_WRITE);
	IDE_disk_wait();
	for(int i = 0; i < SECTOR_SIZE/4 ; i++){
		 _out_data32(ATA_PORT_DATA,((u32*)_global_buf)[i]);
	}
}

void IDE_read2buf(int lba)
{
	for(int i=0;i<512;i++)_global_buf[i]=0;
	switch_to_disk(1);
	set_disk_no(1);/*disk1 :PC hard disk */
	IDE_disk_wait();
	outb_wait(ATA_PORT_SECT_COUNT,1);	/*outb(0x1f2,1);*/
	outb_wait(ATA_PORT_LBA_LOW ,lba);	/*outb(0x1f3,lba);*/
	outb_wait(ATA_PORT_LBA_MID ,lba >> 8);	/*outb(0x1f4,lba>>8)*/
	outb_wait(ATA_PORT_LBA_HIGH ,lba >> 16);/*outb(0x1f5,lba>>16)*/
	outb_wait(ATA_PORT_DEVICE , 0xe0 |(disk_no&1)<<4 | (lba >> 24));/*outb(0x1f5,lba>>16)*/
	/*disk_no determines read which disk.*/
	outb_wait(ATA_PORT_STATUS, HD_READ );
	IDE_disk_wait();
	for(int i = 0; i < SECTOR_SIZE/4 ; i++){
		 ((u32*)_global_buf)[i]=_in_data32(ATA_PORT_DATA);
	}
}
