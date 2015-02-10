#include "ramdisk_fs.h"

static rd_superblock *superblock;
static rd_inode *inode_list;
static char *first_block;			/* first block addr of the whole ramdisk */
static char *first_inodes_block;	/* first block addr of inodes region */
static char *first_bitmap_block;	/* first block addr of bitmap region */
static char *first_data_block;		/* first block addr of data region */

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
	allocate_block();
	allocate_block();
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
		inode_list[i].file_size = 0;
		memset(inode_list[i].block_addr, 0, sizeof(inode_list[i].block_addr));
		inode_list[i].first_level_index = NULL;
		inode_list[i].second_level_index = NULL;
		inode_list[i].third_level_index = NULL;
	}

	inode_list[0].file_type = RD_DIRECTORY;
	inode_list[0].block_count = 1;
	inode_list[0].file_size = sizeof(rd_dentry) * 2; /* "." and ".."'s dentry' */
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
	rd_dentry *parent_dentry;	/* for ".." */
	rd_dentry *self_dentry;		/* for "." */

	memset(first_data_block, 0, RD_DATA_BLOCKS_SIZE);
	self_dentry = (rd_dentry*)first_data_block;
	self_dentry->inode_num = 0;
	strcpy(self_dentry->filename, ".");

	parent_dentry = self_dentry + 1;
	parent_dentry->inode_num = 0;
	strcpy(parent_dentry->filename, "..");

	return 0;
}

int ramfs_exit(void) {
	if (first_block) {
		vfree(first_block);
	}
	first_block = NULL;
	return 0;
}

/*
 * Allocate a free inode. Find a free inode, return it
 */
rd_inode* allocate_inode() {
	int i;

	if (superblock->freeinode_count <= 0) {
		return NULL;
	}
	for (i = 0; i < RD_INODE_NUM; ++i) {
		if (inode_list[i].file_type == RD_AVAILABLE) {
			superblock->freeinode_count--;
			return inode_list + i;
		}
	}
	return NULL;
}

/*
 * Allocate a free block. Find a free block, set its bitmap and return its addr.
 */
char* allocate_block() {
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

	return block_addr;

}

void free_inode(rd_inode *inode) {
	// TODO
	inode->file_type = RD_AVAILABLE;
	
}

void free_block(char *block) {
	int bitmap_index;
	bitmap_index = (int)(block - first_data_block) / RD_BLOCK_SIZE;
	*(first_bitmap_block + bitmap_index) = 0;
}

/*
 * Parse the given ABSOLUTE file path, get the file's upper dir's inode,
 * and the filename. If the path is not valid, the 
 * function returns -1, otherwise 0.
 */
int parse_path(const char *path, rd_inode **parent_inode, char *filename) {
	char* tmp;
	char* next_dir;
	char* block;

	rd_inode* cur_inode;
	rd_inode* par_inode;

	bool found = true; // if the path if '/', return true
	int size_count;
	int i, j;
	int dir_num;

	tmp = (char *)vmalloc(strlen(path));
	strcpy(tmp, path);


	cur_inode = inode_list;
	par_inode = cur_inode;
	next_dir = strsep(&tmp, "/");

	while (next_dir != NULL) {
		
		
		if (cur_inode->file_type != RD_DIRECTORY) return -1;

		// Current file is a directory
		for (i = 0, size_count = 0, found = false; i < cur_inode->block_count; ++i) {

			// TODO: multi-level index

			block = cur_inode->block_addr[i];
			
			dir_num = RD_BLOCK_SIZE / sizeof(rd_dentry);
			for (j = 0; j < dir_num; ++i) {
				rd_dentry* dentry = (rd_dentry*) (block + j*sizeof(rd_dentry));
				if (strcmp(dentry->filename, next_dir) == 0) {
					par_inode = cur_inode;
					cur_inode = inode_list + dentry->inode_num;
					found = true;
					break;
				}

				// ensure not visiting over the border
				size_count += sizeof(rd_dentry);
				if (size_count > cur_inode->file_size) break;
			}
			if (found) break;
			if (size_count > cur_inode->file_size) break;
		}

		strcpy(filename, next_dir);
		next_dir = strsep(&tmp, "/");

		// A directory in the middle of the path not found
		if (!found && next_dir != NULL) return -1;
    }

    *parent_inode = par_inode;
	
	if (found) return 1;
	else return 0;
}

/*
 * Add a file's dentry to its parent's dir file.
 *
 */
int add_dentry(rd_inode *parent_inode, int inode_num, char *filename) {
	// TODO
	return 0;
}

/* 
 * Create a file according to the given ABSOLUTE path
 * Eg: ramfs_create("/a.txt")
 */
int ramfs_create(const char *path) {
	rd_inode *parent_inode;
	rd_inode *file_inode;
	char *file_block;
	char *filename;
	rd_dentry *dentry;
	int ret;

	dentry = NULL;
	filename = NULL;

	if (parse_path(path, &parent_inode, filename) == -1) {
		printk("Error: Invalid Path.\n");
		return -1;
	}

	/* Allocat a block for the file */
	file_block = allocate_block();
	if (file_block == NULL) {
		printk("Error: No free blocks available.\n");
		return -1;
	}
	/* Allocate a inode for the file */
	file_inode = allocate_inode();

	if (file_inode == NULL) {
		printk("Error: No free inodes available.\n");
		free_block(file_block);
		return -1;
	}

	/* Init this inode */
	file_inode->file_type = RD_FILE;
	file_inode->block_count = 1;
	file_inode->block_addr[0] = file_block;

	/* Add a dentry to its parent */
	ret = add_dentry(parent_inode, file_inode->inode_num, filename);
	if (ret == -1) {
		printk("Error: Cannot add dentry.\n");
		free_inode(file_inode);
		free_block(file_block);
		return -1;
	}

	return 0;

}
