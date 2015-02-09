#include "ramdisk_fs.h"

static rd_superblock *superblock;
static rd_inode *inode_list;
static char *first_block;
static char *first_inodes_block;
static char *first_bitmap_block;
static char *first_data_block;

int ramfs_init(void) {

	first_block = (char *)vmalloc(RD_DISK_SIZE);

	if (!first_block) {
		printk("Error: Ramdisk Memory Allocation Failed.\n");
		return -1;
	} else {
		printk("Ramdisk Memory Allocated.\n");
	}

	first_inodes_block = first_block + RD_SUPERBLOCK_SIZE;
	first_bitmap_block = first_inodes_block + RD_INODES_SIZE;
	first_data_block = first_bitmap_block + RD_BLOCKBITMAP_SIZE;

	superblock = (rd_superblock*)first_block;
	inode_list = (rd_inode*)first_inodes_block;

	superblock_init();
	inodes_init();
	bitmap_init();
	data_init();
	allocate_block(inode_list);
	allocate_block(inode_list);
	return 0;
}

int superblock_init(void) {
	superblock->block_count = RD_BLOCK_NUM;
	superblock->inode_count = RD_INODE_NUM;
	superblock->freeblock_count = RD_BLOCK_NUM;
	superblock->freeinode_count = RD_INODE_NUM;
	superblock->first_inodes_block = first_inodes_block;
	superblock->first_bitmap_block = first_bitmap_block;
	superblock->first_data_block = first_data_block;
	return 0;
}

int inodes_init(void) {
	int i;
	for (i = 1; i < RD_INODE_NUM; ++i) {
		inode_list[i].inode_num = i;
		inode_list[i].file_type = RD_AVAILABLE;
		inode_list[i].block_count = 0;
		memset(inode_list[i].block_addr, 0, sizeof(inode_list[i].block_addr));
		inode_list[i].first_level_index = NULL;
		inode_list[i].second_level_index = NULL;
		inode_list[i].third_level_index = NULL;
	}

	inode_list[0].file_type = RD_DIRECTORY;
	inode_list[0].block_count = 1;
	inode_list[0].block_addr[0] = first_data_block;
	return 0;
}

int bitmap_init(void) {

	memset(first_bitmap_block, 0, RD_BLOCKBITMAP_SIZE);
	/* block 0 is allocated for root dir*/
	*first_bitmap_block = 1;

	return 0;
}

int data_init(void) {

	memset(first_data_block, 0, RD_DATA_BLOCKS_SIZE);
	// TODO: init root dir's data block
	return 0;
}



rd_inode* find_free_inode(void) {
	int i;

	if (superblock->freeinode_count <= 0)
		return NULL;
	for (i = 0; i < RD_INODE_NUM; ++i) {
		if (inode_list[i].file_type == RD_AVAILABLE)
			break;
	}
	return inode_list + i;
}

char* allocate_block(rd_inode *inode) {
	int i, j, block_num;
	char byte;
	char *iter;
	char *block_addr;

	if (superblock->freeblock_count <= 0) {
		printk("Error: No free blocks available.\n");
		return NULL;
	}

	iter = first_bitmap_block;
	for (i = 0; i < RD_BLOCKBITMAP_SIZE; ++iter, ++i) {
		byte = *iter;
		for (j = 0; j < 8; ++j) {
			if (((byte >> j) & 1) == 0)
				break;
		}
		if (j < 8) {
			/* set the bitmap */
			*iter = byte | (1 << j);
			break;
		}
	}

	block_num = i * 8 + j;
	block_addr = first_data_block + block_num * RD_BLOCK_SIZE;
	superblock->freeblock_count--;
	printk("Block %d allocated, Addr: %p, Remaining: %d.\n", block_num, block_addr, superblock->freeblock_count);

	inode->block_count++;
	if (inode->block_count <= 10) {
		inode->block_addr[inode->block_count - 1] = block_addr;
	}
	// TO-DO: 1st/2nd/3rd level index
	return block_addr;

}

int ramfs_exit(void) {
	if (first_block) {
		vfree(first_block);
	}
	first_block = NULL;
	return 0;
}