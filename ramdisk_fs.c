#include "ramdisk_fs.h"

static rd_superblock *superblock;
static rd_inode *inode_list;
static char *first_block;
static char *first_inodes_block;
static char *first_bitmap_block;
static char *first_data_block;

int init_fs(void) {

	first_block = (char *)vmalloc(RD_DISK_SIZE);

	if (!first_block) {
		printk("Ramdisk Memory Allocation Failed.\n");
		return -1;
	} else {
		printk("Ramdisk Memory Allocated.\n");
	}

	first_inodes_block = first_block + RD_SUPERBLOCK_SIZE;
	first_bitmap_block = first_inodes_block + RD_INODES_SIZE;
	first_data_block = first_bitmap_block + RD_BLOCKBITMAP_SIZE;

	superblock = (rd_superblock*)first_block;
	inode_list = (rd_inode*)first_inodes_block;

	init_superblock();
	init_inodes();
	init_bitmap();
	init_data_blocks();
	return 0;
}

int init_superblock(void) {
	superblock->block_count = RD_BLOCK_NUM;
	superblock->inode_count = RD_INODE_NUM;
	superblock->freeblock_count = RD_BLOCK_NUM;
	superblock->freeinode_count = RD_INODE_NUM;
	superblock->first_inodes_block = first_inodes_block;
	superblock->first_bitmap_block = first_bitmap_block;
	superblock->first_data_block = first_data_block;
	return 0;
}

int init_inodes(void) {
	int i;
	for (i = 1; i < RD_INODE_NUM; ++i) {
		inode_list[i].inode_num = i;
		inode_list[i].file_type = RD_AVAILABLE;
		inode_list[i].block_num = 0;
		memset(inode_list[i].block_addr, 0, sizeof(inode_list[i].block_addr));
		inode_list[i].first_level_index = NULL;
		inode_list[i].second_level_index = NULL;
		inode_list[i].third_level_index = NULL;
	}

	inode_list[0].file_type = RD_DIRECTORY;
	inode_list[0].block_num = 1;
	inode_list[0].block_addr[0] = first_data_block;
	return 0;
}

int init_bitmap(void) {
	memset(first_bitmap_block, 0, RD_BLOCKBITMAP_SIZE);
	return 0;
}

int init_data_blocks(void) {
	memset(first_data_block, 0, RD_DATA_BLOCKS_SIZE);
	// TODO: init root dir's data block
	return 0;
}

int exit_fs(void) {
	if (first_block) {
		vfree(first_block);
	}
	first_block = NULL;
	return 0;
}