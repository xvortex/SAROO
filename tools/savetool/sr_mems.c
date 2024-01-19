
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "bup.h"


/******************************************************************************/

static u8 *mems_buf;

/******************************************************************************/

typedef struct {
	u32 magic0;
	u32 magic1;
	u32 total_size;
	u16 free_block;
	u16 first_save;
	u8  bitmap[1024-16];
}MEMSHEADER;

#define MEMS  ((MEMSHEADER*)(mems_buf))

static u16 block_size = 1024;
static u16 free_block;

/******************************************************************************/


static int get_free_block(int pos)
{
	int i, byte, mask;

	for(i=pos; i<8064; i++){
		byte = i/8;
		mask = 1<<(i&7);
		if( (MEMS->bitmap[byte]&mask) == 0 ){
			MEMS->bitmap[byte] |= mask;
			free_block -= 1;
			//printf("get_free_block(%d): %d\n", pos, i);
			return i;
		}
	}

	return 0;
}


static u8 *get_block_addr(int id)
{
	return (u8*)(mems_buf + block_size*id);
}


static int access_data(int block, u8 *data, int type)
{
	u8 *bp, *bmp;
	int dsize, asize;

	bp = get_block_addr(block);
	dsize = get_be32(bp+0x0c);
	if(dsize<=(block_size-64)){
		if(type==0){	// read
			memcpy(data, bp+0x40, dsize);
		}else{			// write
			memcpy(bp+0x40, data, dsize);
		}
		return 0;
	}

	bmp = bp+0x40;
	while(dsize>0){
		block = get_be16(bmp);
		bmp += 2;
		bp = get_block_addr(block);

		asize = (dsize>block_size)? block_size : dsize;
		//printf("dsize=%04x block=%04x asize=%04x\n", dsize, block, asize);

		if(type==0){	// read
			memcpy(data, bp, asize);
		}else{			// write
			memcpy(bp, data, asize);
		}
		data += asize;
		dsize -= asize;
	}

	return 0;
}


/******************************************************************************/


int sr_mems_export(int slot_id, int save_id)
{
	int block, i, index;
	u8 *bp;

	index = 0;
	for(i=0; i<448; i++){
		bp = (u8*)(mems_buf+1024+i*16);
		if(bp[0]==0)
			continue;
		if(index == save_id){
			block = get_be16(bp+0x0e);
			bp = get_block_addr(block);

			memset(save_buf, 0, 0x100000);

			strcpy((char*)save_buf, "SSAVERAW");
			memcpy(save_buf+0x10, bp+0, 11);    // Filename
			memcpy(save_buf+0x1c, bp+0x0c, 4);  // Size
			memcpy(save_buf+0x20, bp+0x10, 10); // Comment
			save_buf[0x2b] = bp[0x1b];          // Language
			memcpy(save_buf+0x2c, bp+0x1c, 4);  // Date

			access_data(block, save_buf+0x40, 0);

			char fname[16];
			int fsize = get_be32(save_buf+0x1c);
			sprintf(fname, "%s.bin", save_buf+0x10);
			write_file(fname, save_buf, fsize+0x40);

			printf("Export Save_%d: %s\n", index, fname);
			return 0;
		}
		index += 1;
	}

	printf("Save_%d not found!\n", save_id);
	return -1;
}


int sr_mems_import(int slot_id, int save_id, char *save_name)
{
	SAVEINFO *save;
	int block, block_need, i, index, last, hdr;
	u8 *bp;

	index = 0;
	block = 0;
	last = -1;
	for(i=0; i<448; i++){
		bp = (u8*)(mems_buf+1024+i*16);
		if(bp[0]==0){
			if(last==-1){
				last = i;
			}
			continue;
		}
		if(index == save_id){
			block = get_be16(bp+0x0e);
			break;
		}
		index += 1;
	}

	if(block){
		// overwrite
		char sname[20];
		if(save_name==NULL){
			sprintf(sname, "%s.bin", bp);
			save_name = sname;
		}
		save = load_saveraw(save_name);
		if(save==NULL)
			return -1;

		*(u32*)(bp+0x1c) = save->date;
		access_data(block, save->dbuf, 2);
		printf("Import %s from %s.\n", bp, save_name);
		return 0;
	}
	if(save_id>=0){
		printf("Save_%d not found!\n", save_id);
		return -1;
	}

	// new save
	save = load_saveraw(save_name);
	if(save==NULL)
		return -1;

	// ��������Ŀ���������ʼ��+���ݿ�
	int dsize = get_be32(&save->data_size);
	if(dsize<=(block_size-64)){
		block_need = 0;
	}else{
		block_need = (dsize+block_size-1)/(block_size);
		if((block_need+1) > free_block){
			return -1;
		}
	}
	printf("block_need=%d\n", block_need+1);

	// ������ʼ��
	block = get_free_block(0);
	hdr = block;

	bp = get_block_addr(hdr);
	printf("start at %d\n", hdr);

	// д��ʼ��
	memset(bp, 0, 1024);
	memcpy(bp+0x00, save->file_name, 11);
	*(u32*)(bp+0x0c) = save->data_size;
	memcpy(bp+0x10, save->comment, 10);
	bp[0x1b] = save->language;
	*(u32*)(bp+0x1c) = save->date;

	// �����
	block = 0;
	for(i=0; i<block_need; i++){
		block = get_free_block(block);
		put_be16(bp+0x40+i*2, block);
		block += 1;
	}
	put_be16(bp+0x40+i*2, 0);

	// д����
	access_data(hdr, save->dbuf, 2);

	// ����Ŀ¼
	memcpy((u8*)mems_buf+1024+last*16, save->file_name, 11);
	put_be16(mems_buf+1024+last*16+0x0e, hdr);

	put_be16(&MEMS->free_block, free_block);

	printf("Import %s.\n", save->file_name);
	return 0;
}


int sr_mems_delete(int slot_id, int save_id)
{
	int block, i, index, last;
	u8 *bp;

	index = 0;
	block = 0;
	last = -1;
	for(i=0; i<448; i++){
		bp = (u8*)(mems_buf+1024+i*16);
		if(bp[0]==0)
			continue;
		if(index == save_id){
			block = get_be16(bp+0x0e);
			last = i;
			break;
		}
		index += 1;
	}
	if(block==0){
		printf("Save_%d not found!\n", save_id);
		return -1;
	}

	bp = get_block_addr(block);

	// �ͷſ�ʼ��
	set_bitmap((u8*)MEMS->bitmap, block, 0);
	free_block += 1;

	// �ͷ����ݿ�
	int dsize = get_be32(bp+0x0c);
	if(dsize>960){
		u8 *bmp = bp+0x40;
		block = 0;
		while(1){
			block = get_be16(bmp);
			if(block==0)
				break;
			set_bitmap((u8*)MEMS->bitmap, block, 0);
			free_block += 1;
			bmp += 2;
		}
	}

	// ����lastָ��
	memset((u8*)mems_buf+1024+last*16, 0, 16);

	put_be16(&MEMS->free_block, free_block);

	printf("Delete Save_%d.\n", save_id);

	return 0;
}


int sr_mems_create(char *game_id)
{
	return -1;
}


int sr_mems_list(int slot_id)
{
	int block, i, index, dsize, bsize;
	u8 *bp;

	index = 0;
	for(i=0; i<448; i++){
		bp = (u8*)(mems_buf+1024+i*16);
		if(bp[0]==0)
			continue;

		block = get_be16(bp+0x0e);
		bp = get_block_addr(block);

		dsize = get_be32(bp+0x0c);
		bsize = 0;
		if(dsize>960){
			bsize = (dsize+block_size-1)/(block_size);
		}
		bsize += 1;

		printf("  Save_%-3d: %s  size=%5x  block=%d\n", index, bp, dsize, bsize);

		index += 1;
	}
	printf("\n");

	printf(" Total block: %4d   Free block: %d\n\n", 8064, free_block);
	return 0;
}


int sr_mems_init(u8 *bbuf)
{
	mems_buf = bbuf;
	free_block = get_be16(&MEMS->free_block);

	return 0;
}


/******************************************************************************/

