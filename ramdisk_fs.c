#include "ramdisk_fs.h"
#include "ramdisk_defs.h"

static rd_superblock *superblock;
static rd_inode *inode_list;
static char *first_block;			/* first block addr of the whole ramdisk */
static char *first_inodes_block;	/* first block addr of inodes region */
static char *first_bitmap_block;	/* first block addr of bitmap region */
static char *first_data_block;		/* first block addr of data region */

static rd_file **fd_list;

/*
 * Init the whole ramdisk, allocate memory and init all the 4 memory regions
 */
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
	fdt_init();
	return 0;
}

/*
 * Init the superblock region
 */
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

/*
 * Init the inodes region
 */
int inodes_init(void) {
	int i;
	for (i = 0; i < RD_INODE_NUM; ++i) {
		inode_list[i].inode_num = i;
		inode_list[i].file_type = RD_AVAILABLE;
		inode_list[i].block_count = 0;
		inode_list[i].file_size = 0;
		memset(inode_list[i].block_addr, 0, sizeof(inode_list[i].block_addr));
	}

	inode_list[0].file_type = RD_DIRECTORY;
	inode_list[0].block_count = 1;
	inode_list[0].block_addr[0] = first_data_block;
	superblock->freeblock_count--;
	superblock->freeinode_count--;
	return 0;
}

/*
 * Init the bitmap region
 */
int bitmap_init(void) {

	memset(first_bitmap_block, 0, RD_BLOCKBITMAP_SIZE);
	/* block 0 is allocated for root dir*/
	*first_bitmap_block = 1;

	return 0;
}

/*
 * Init the data region
 */
int data_init(void) {

	memset(first_data_block, 0, RD_DATA_BLOCKS_SIZE);

	add_dentry(&inode_list[0], 0, ".");
	add_dentry(&inode_list[0], 0, "..");

	return 0;
}

/*
 * Init the File Descriptor Table (FDT)
 */
int fdt_init(void) {
	int i;
	fd_list = (rd_file**)vmalloc(sizeof(rd_file*) * RD_MAX_FILE);
	for (i = 0; i < RD_MAX_FILE; ++i) {
		fd_list[i] = NULL;
	}
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
 * Allocate a free fd.
 */
int allocate_fd() {
	int i;
	for (i = 0; i < RD_MAX_FILE; ++i) {
		if (fd_list[i] == NULL) {
			fd_list[i] = (rd_file*)vmalloc(sizeof(rd_file));
			return i;
		}
	}
	/* No free fd available */
	return -1;
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
	return block_addr;

}

/*
 * Free the given inode
 */
void free_inode(rd_inode *inode) {
	
	inode->file_type = RD_AVAILABLE;
	inode->block_count = 0;
	inode->file_size = 0;
	memset(inode->block_addr, 0, sizeof(inode->block_addr));
	superblock->freeinode_count++;
}

/*
 * Free the given fd
 */
void free_fd(int fd) {
	vfree(fd_list[fd]);
	fd_list[fd] = NULL;
}

/*
 * Free the given block
 */
void free_block(char *block) {
	int block_num;
	int i, j;
	char *byte;
	block_num = (block - first_data_block) / RD_BLOCK_SIZE;
	i = block_num / 8;
	j = block_num % 8;
	byte = first_bitmap_block + i;
	*byte = (*byte) & (~(1 << j));
	superblock->freeblock_count++;
}

void free_dentry(rd_dentry *dentry) {
	dentry->inode_num = -1;
	memset(dentry->filename, 0, sizeof(dentry->filename));
}

/*
 * Parse the given ABSOLUTE file path
 * If the file exists, get its inode, its parent's inode and its filename, return 1
 * If not, get its parent's inode and its filename, return 0
 * If the path is not valid ,return -1
 */
int parse_path(const char *path, int type, rd_inode **parent_inode, rd_inode **file_inode, char *filename) {
	char* tmp;
	char* next_dir;
	char* block;

	rd_inode* cur_inode;
	rd_inode* par_inode;

	bool found = true; // if the path if '/', return true
	int size_count;
	int i, j;
	int dir_num;


	*parent_inode = NULL;
	*file_inode = NULL;

	/* if the path doesn't start with '/', invalid path*/
	if (path[0] != '/') {
		return -1;
	}

	/* if the path is a file path but ends with '/', invalid path */
	if (type == RD_FILE && path[strlen(path)-1] == '/') {
		return -1;
	}
	tmp = (char *)vmalloc(strlen(path));
	strcpy(tmp, path);


	cur_inode = inode_list;
	par_inode = cur_inode;
	next_dir = strsep(&tmp, "/");
	next_dir = strsep(&tmp, "/");
	while (next_dir != NULL && strlen(next_dir) != 0) {
		if (cur_inode->file_type != RD_DIRECTORY) return -1;

		// Current file is a directory
		for (i = 0, size_count = 0, found = false; i < cur_inode->block_count; ++i) {
			block = cur_inode->block_addr[i];
			
			dir_num = RD_BLOCK_SIZE / sizeof(rd_dentry);
			for (j = 0; j < dir_num; ++j) {
				rd_dentry* dentry = (rd_dentry*) (block + j*sizeof(rd_dentry));

				if (strcmp(dentry->filename, next_dir) == 0) {
					par_inode = cur_inode;
					cur_inode = inode_list + dentry->inode_num;
					found = true;
					break;
				}

				// ensure not visiting over the border
				size_count += sizeof(rd_dentry);
				if (size_count >= cur_inode->file_size) break;
			}
			if (found) break;
			if (size_count >= cur_inode->file_size) break;
		}
		strcpy(filename, next_dir);
		next_dir = strsep(&tmp, "/");

		// A directory in the middle of the path not found
		if (!found && next_dir != NULL && strlen(next_dir) != 0) return -1;
    }
	if (found) {
		*parent_inode = par_inode;
		*file_inode = cur_inode;
		return 1;
	}
	else  {
		*parent_inode = cur_inode;
		return 0;
	}
}


/*
 * Add a file's dentry to its parent's dir file.
 * First check if there is some dentry marked invalid in this dir file, if yes,
 * use this dentry, otherwise allocate some space for a new dentry
 */
int add_dentry(rd_inode *parent_inode, int inode_num, char *filename) {
	int parent_file_size;
	int parent_block_count;
	char *parent_last_block;
	rd_dentry *dentry;
	int offset, i, j, size_count, max_dentry_num;
	parent_file_size = parent_inode->file_size;
	parent_block_count = parent_inode->block_count;
	max_dentry_num = RD_BLOCK_SIZE / sizeof(rd_dentry);
	offset = parent_file_size % RD_BLOCK_SIZE;
	size_count = 0;
	/* find if there are some invalid dentry(file deleted) */
	for (i = 0; i < parent_inode->block_count; ++i) {
		dentry = (rd_dentry*)parent_inode->block_addr[i];
		for (j = 0; j < max_dentry_num; ++j) {
			if (dentry->inode_num == -1) {
				dentry->inode_num = inode_num;
				strcpy(dentry->filename, filename);
				return 0;
			}
			size_count += sizeof(rd_dentry);
			if (size_count >= parent_inode->file_size)
				break;
			dentry++; 
		}
		if (size_count >= parent_inode->file_size)
			break;
		size_count += RD_BLOCK_SIZE - (size_count % RD_BLOCK_SIZE);
	}

	/* find the last block, if not enough block size remains, allocate a new block */
	if (parent_block_count == RD_MAX_FILE_BLK) {
		printk("Error: Failed to add dentry, max file size reached.\n");
		return -1;
	}
	if (offset + sizeof(rd_dentry) > RD_BLOCK_SIZE) {
		parent_inode->file_size += RD_BLOCK_SIZE - offset;
		parent_inode->block_addr[parent_block_count] = allocate_block();
		parent_last_block = parent_inode->block_addr[parent_block_count];
		if (parent_last_block == NULL) {
			printk("Error: Failed to add dentry, no free blocks available.\n");
			return -1;
		}
		parent_inode->block_count++;
		
		offset = 0;
	} else {
		parent_last_block = parent_inode->block_addr[parent_block_count-1];
	}
	/* write the dentry */
	dentry = (rd_dentry*)(parent_last_block + offset);
	dentry->inode_num = inode_num;
	strcpy(dentry->filename, filename);
	parent_inode->file_size += sizeof(rd_dentry);
	return 0;
}

/*
 * Get the dentry of the given path
 */
rd_dentry* get_dentry(const char *path) {
	rd_dentry *dentry;
	rd_inode *par_inode;
	rd_inode *file_inode;
	char *filename;
	int ret, i, j, dir_num, size_count;


	filename = (char*)vmalloc(RD_MAX_FILENAME);
	ret = parse_path(path, RD_FILEORDIR, &par_inode, &file_inode, filename);
	dir_num = RD_BLOCK_SIZE / sizeof(rd_dentry);
	size_count = 0;
	if (ret == -1) {
		printk("Error: Invalid path %s.\n", path);
		return NULL;
	} else if (ret == 0) {
		printk("Error: File '%s' doesn't exist.\n", path);
		return NULL;
	}

	for (i = 0; i < par_inode->block_count; ++i) {
		dentry = (rd_dentry*)par_inode->block_addr[i];
		for (j = 0; j < dir_num; ++j) {
			if (dentry->inode_num == file_inode->inode_num) {
				return dentry;
			}
			size_count += sizeof(rd_dentry);
			if (size_count >= par_inode->file_size)
				break;
			dentry++;
		}
	}

	return NULL;
}

/* 
 * Create a file according to the given ABSOLUTE path
 */
int ramfs_create(const char *path, char *msg) {
	rd_inode *parent_inode;
	rd_inode *file_inode;
	char *file_block;
	char *filename;
	rd_dentry *dentry;
	int ret;

	dentry = NULL;
	filename = (char*)vmalloc(RD_MAX_FILENAME);
	ret = parse_path(path, RD_FILE, &parent_inode, &file_inode, filename);
	if (ret == -1) {
		sprintf(msg + strlen(msg), "Error: Invalid Path '%s'.\n", path);
		return -1;
	} else if (ret == 1) {
		sprintf(msg + strlen(msg), "Error: File '%s' already exists.\n", path);
		return -1;
	}


	/* Allocate a block for the file */
	file_block = allocate_block();
	if (file_block == NULL) {
		sprintf(msg + strlen(msg), "Error: No free blocks available.\n");
		return -1;
	}
	/* Allocate a inode for the file */
	file_inode = allocate_inode();

	if (file_inode == NULL) {
		sprintf(msg + strlen(msg), "Error: No free inodes available.\n");
		free_block(file_block);
		return -1;
	}

	/* Init this inode */
	file_inode->file_type = RD_FILE;
	file_inode->file_size = 0;
	file_inode->block_count = 1;
	file_inode->block_addr[0] = file_block;

	/* Add a dentry to its parent */
	ret = add_dentry(parent_inode, file_inode->inode_num, filename);
	if (ret == -1) {
		sprintf(msg + strlen(msg), "Error: Parent dir's size reaches max-file-size.\n");
		free_inode(file_inode);
		free_block(file_block);
		return -1;
	}

	sprintf(msg + strlen(msg), "Successfully create '%s'.\n", path);
	return 0;

}

/*
 * Make a new directory according to the given path.
 */
int ramfs_mkdir(const char *path, char *msg) {
	rd_inode *parent_inode;
	rd_inode *file_inode;
	char *file_block;
	char *filename;
	rd_dentry *dentry;
	int ret;

	dentry = NULL;
	filename = (char*)vmalloc(RD_MAX_FILENAME);
	ret = parse_path(path, RD_DIRECTORY, &parent_inode, &file_inode, filename);
	if (ret == -1) {
		sprintf(msg + strlen(msg), "Error: Invalid Path '%s'.\n", path);
		return -1;
	} else if (ret == 1) {
		sprintf(msg + strlen(msg), "Error: File '%s' already exists.\n", path);
		return -1;
	}

	/* Allocate a block for the file */
	file_block = allocate_block();
	if (file_block == NULL) {
		sprintf(msg + strlen(msg), "Error: No free blocks available.\n");
		return -1;
	}
	/* Allocate a inode for the file */
	file_inode = allocate_inode();

	if (file_inode == NULL) {
		sprintf(msg + strlen(msg), "Error: No free inodes available.\n");
		free_block(file_block);
		return -1;
	}

	/* Init this inode */
	file_inode->file_type = RD_DIRECTORY;
	file_inode->file_size = 0;
	file_inode->block_count = 1;
	file_inode->block_addr[0] = file_block;

	/* Add a dentry to its parent */
	ret = add_dentry(parent_inode, file_inode->inode_num, filename);
	if (ret == -1) {
		sprintf(msg + strlen(msg), "Error: Cannot add dentry.\n");
		free_inode(file_inode);
		free_block(file_block);
		return -1;
	}

	/* Add . .. dentry */
	ret = add_dentry(file_inode, file_inode->inode_num, ".");
	if (ret == -1) {
		sprintf(msg + strlen(msg), "Error: Cannot add dentry.\n");
		free_inode(file_inode);
		free_block(file_block);
		return -1;		
	}

	ret = add_dentry(file_inode, parent_inode->inode_num, "..");
	if (ret == -1) {
		sprintf(msg + strlen(msg), "Error: Cannot add dentry.\n");
		free_inode(file_inode);
		free_block(file_block);
		return -1;		
	}	

	sprintf(msg + strlen(msg), "Successfully mkdir '%s'.\n", path);
	return 0;	
}

/*
 * Delete a regular file according to the given path
 */
int ramfs_delete(const char *path, char *msg) {
	rd_inode *parent_inode;
	rd_inode *file_inode;
	char *filename;
	rd_dentry *dentry;
	int ret, i;

	dentry = NULL;
	filename = (char*)vmalloc(RD_MAX_FILENAME);
	ret = parse_path(path, RD_FILE, &parent_inode, &file_inode, filename);
	if (ret == -1) {
		sprintf(msg + strlen(msg), "Error: Invalid path '%s'.\n", path);
		return -1;
	} else if (ret == 0) {
		sprintf(msg + strlen(msg), "Error: Path '%s' doesn't exist.\n", path);
		return -1;
	} else if (file_inode->file_type != RD_FILE) {
		sprintf(msg + strlen(msg), "Error: Path '%s' is not a regular file.\n", path);
		return -1;
	}


	for (i = 0; i < RD_MAX_FILE; ++i) {
		if (fd_list[i] != NULL && fd_list[i]->inode == file_inode) {
			free_fd(i);
		}
	}

	dentry = get_dentry(path);
	free_dentry(dentry);
	for (i = 0; i < file_inode->block_count; ++i)
		free_block(file_inode->block_addr[i]);
	free_inode(file_inode);
	sprintf(msg + strlen(msg), "Successfully delete '%s'.\n", path);
	return 0;
}
/* 
 * Open a file according to the given path
 * Allocate a new fd for this file, then return the fd.
 */
int ramfs_open(const char *path, int mode, char *msg) {
	int ret, fd;
	rd_inode *par_inode;
	rd_inode *file_inode;
	rd_file *file;
	char *filename;

	filename = (char *)vmalloc(RD_MAX_FILENAME);


	ret = parse_path(path, RD_FILE, &par_inode, &file_inode, filename);

	if (ret == -1) {
		sprintf(msg + strlen(msg), "Error: Invalid path '%s'.\n", path);
		return -1;
	} else if (ret == 0) {
		sprintf(msg + strlen(msg), "Error: Path '%s' doesn't exist.\n", path);
		return -1;
	} else if (file_inode->file_type != RD_FILE) {
		sprintf(msg + strlen(msg), "Error: Path '%s' is not a regular file.\n", path);
		return -1;
	}
	/* allocate a new fd */

	fd = allocate_fd();
	if (fd == -1) {
		sprintf(msg + strlen(msg), "Error: No free fd available.\n");
		return -1;
	}

	file = fd_list[fd];
	strcpy(file->path, path);
	file->inode = file_inode;
	file->dentry = get_dentry(path);
	file->offset = 0;
	file->mode = mode;

	sprintf(msg + strlen(msg), "Successfully open '%s'.\n", path);
	return fd;
}

/*
 * Close a file according to the given fd.
 */
int ramfs_close(int fd, char *msg) {
	
	if (fd < 0 || fd >= RD_MAX_FILE) {
		sprintf(msg + strlen(msg), "Error: Invalid fd '%d'.\n", fd);
		return -1;
	}

	if (fd_list[fd] == NULL) {
		sprintf(msg + strlen(msg), "Error: Invalid fd '%d'.\n", fd);
		return -1;
	}
	free_fd(fd);
	sprintf(msg + strlen(msg), "Successfully close '%d'.\n", fd);
	return 0;
}

/*
 * Read a file according to the given fd.
 * return the number of bytes that are successfully read.
 */
int ramfs_read(int fd, char *buf, size_t count, char *msg) {
	rd_file *file;
	rd_inode *inode;
	char *byte;
	int offset, blknum, blkoffset, i, read_cnt;

	if (fd < 0 || fd >= RD_MAX_FILE) {
		sprintf(msg + strlen(msg), "Error: Invalid fd %d.\n", fd);
		return -1;
	}

	file = fd_list[fd];
	/* check if the fd is valid */
	if (file == NULL) {
		sprintf(msg + strlen(msg), "Error: Invalid fd '%d'.\n", fd);
		return -1;
	}
	/* check if the file is write-only */
	if (file->mode == RD_WRONLY) {
		sprintf(msg + strlen(msg), "Error: Write only file '%s'.\n", file->path);
		return -1;
	}
	offset = file->offset;
	inode = file->inode;
	/* check if the file reaches the max-file-size */
	if (offset == inode->file_size) {
		return 0;
	}

	blknum = offset / RD_BLOCK_SIZE;
	blkoffset = offset % RD_BLOCK_SIZE;
	byte = inode->block_addr[blknum] + blkoffset;
	read_cnt = 0;

	for (i = 0; i < count; ++i) {
		*(buf++) = *(byte++);
		read_cnt++;
		offset++;
		if (offset == inode->file_size) {
			break;
		}
		if (offset % RD_BLOCK_SIZE == 0) {
			/* go to the next block */
			blknum++;
			byte = inode->block_addr[blknum];
		}
	}
	file->offset = offset;
	sprintf(msg + strlen(msg), "Successfully read '%d' bytes from fd '%d'.\n", read_cnt, fd);
	return read_cnt;
}

/*
 * Write a file according to the given fd.
 * return the number of bytes that are successfully written.
 */
int ramfs_write(int fd, char *buf, size_t count, char *msg) {

	rd_file *file;
	rd_inode *inode;
	char *byte;
	int offset, blknum, blkoffset, i, write_cnt;
	if (fd < 0 || fd >= RD_MAX_FILE) {
		sprintf(msg + strlen(msg), "Error: Invalid fd %d.\n", fd);
		return -1;
	}
	file = fd_list[fd];
	/* check if the fd is valid */
	if (file == NULL) {
		sprintf(msg + strlen(msg), "Error: Invalid fd '%d'.\n", fd);
		return -1;
	}
	/* check if the file is read-only */
	if (file->mode == RD_RDONLY) {
		sprintf(msg + strlen(msg), "Error: Read only file '%s'.\n", file->path);
		return -1;
	}
	
	offset = file->offset;
	/* check if the file reaches the max-file-size */
	if (offset == RD_MAX_FILE_SIZE) {
		sprintf(msg + strlen(msg), "Warning: Max file size reached.\n");
		return 0;
	}
	inode = file->inode;
	blknum = offset / RD_BLOCK_SIZE;
	blkoffset = offset % RD_BLOCK_SIZE;
	byte = inode->block_addr[blknum] + blkoffset;
	write_cnt = 0;

	for (i = 0; i < count; ++i) {
		*(byte++) = *(buf++);
		write_cnt++;
		offset++;
		if (offset % RD_BLOCK_SIZE == 0) {
			/* go to the next block */
			blknum++;
			if (blknum == inode->block_count) {
				/* check if max block reachd */
				if (inode->block_count == RD_MAX_FILE_BLK) {
					break;
				}
				/* allocate a new block to write */
				byte = allocate_block();
				if (byte == NULL) {
					sprintf(msg + strlen(msg), "Error: No free blocks available.\n");
					break;
				}
				inode->block_addr[inode->block_count++] = byte;
				
			}
			byte = inode->block_addr[blknum];
		}
	}

	inode->file_size += write_cnt;
	file->offset = offset;
	sprintf(msg + strlen(msg), "Successfully write '%d' bytes to fd '%d'.\n", write_cnt, fd);
	return write_cnt;
}

/*
 * lseek (change the offset in fd) a file according to the given fd.
 */
int ramfs_lseek(int fd, int offset, char *msg) {
	rd_file *file;
	rd_inode *inode;
	if (fd < 0 || fd >= RD_MAX_FILE) {
		sprintf(msg + strlen(msg), "Error: Invalid fd '%d'.\n", fd);
		return -1;
	}
	file = fd_list[fd];
	/* check if the fd is valid */
	if (file == NULL) {
		sprintf(msg + strlen(msg), "Error: Invalid fd '%d'.\n", fd);
		return -1;
	}

	inode = file->inode;
	if (offset > inode->file_size) {
		sprintf(msg + strlen(msg), "Error: Offset '%d' is larger than file size '%d'.\n", offset, inode->file_size);
		return -1;
	}
	file->offset = offset;
	sprintf(msg + strlen(msg), "Successfully lseek, current offset of fd '%d' is '%d'.\n", fd, offset);
	return 0;
}

/*
 * Show the status of all valid blocks
 */
int show_blocks_status(char *msg) {
	int i, j;
	char byte;
	sprintf(msg + strlen(msg), "======================Block Status======================\n");
	sprintf(msg + strlen(msg), "Available free blocks: %d. Total: %d\n\n", superblock->freeblock_count, superblock->block_count);
	sprintf(msg + strlen(msg), "BlkNum\tBlkAddr\n");
	for (i = 0; i < RD_BLOCKBITMAP_SIZE; ++i) {
		byte = *(first_bitmap_block + i);
		for (j = 0; j < 8; ++j) {
			if ((byte >> j) & 1) {
				sprintf(msg + strlen(msg), "%d\t%p\n", i * 8 + j, first_data_block + (i * 8 + j) * RD_BLOCK_SIZE);
			}
		}
	}
	sprintf(msg + strlen(msg), "========================================================\n");
	return 0;
}

/*
 * Show the status of all valid inodes
 */
int show_inodes_status(char *msg) {
	int i, j;
	char* dirtype;
	char* filetype;
	char *type;
	sprintf(msg + strlen(msg), "======================Inode Status======================\n");
	sprintf(msg + strlen(msg), "Available free inodes: %d, Total: %d\n\n", superblock->freeinode_count, superblock->inode_count);
	sprintf(msg + strlen(msg), "InodeNum\tType\tBlkCnt\tSize\tBlkAddr\n");



	dirtype = "dir";
	filetype = "file";
	for (i = 0; i < RD_INODE_NUM; ++i) {
		if (inode_list[i].file_type != RD_AVAILABLE) {
			if (inode_list[i].file_type == RD_FILE)
				type = filetype;
			else
				type = dirtype;
			sprintf(msg + strlen(msg), "%d\t\t%s\t%d\t%d\t", inode_list[i].inode_num, 
				                         type,
				                         inode_list[i].block_count,
				                         inode_list[i].file_size);
			for (j = 0; j < RD_MAX_FILE_BLK; ++j) {
				if (inode_list[i].block_addr[j] == NULL)
					break;
				if (j != 0)
					sprintf(msg + strlen(msg), "\t\t\t\t\t");
				sprintf(msg + strlen(msg), "%p\n", inode_list[i].block_addr[j]);
			}
		}

	}
	sprintf(msg + strlen(msg), "========================================================\n");

	return 0;
}

/*
 * Show the status of a directory. List all the files and sub-directories under it.
 */
int show_dir_status(const char *path, char *msg) {
	char *filename;
	rd_inode *par_inode;
	rd_inode *inode;
	int ret, i, j, size_count, max_dentry_num;
	rd_dentry *dentry;

	filename = (char*)vmalloc(RD_MAX_FILENAME);
	ret = parse_path(path, RD_DIRECTORY, &par_inode, &inode, filename);
	if (ret == -1) {
		sprintf(msg + strlen(msg), "Error: Invalid path '%s'.\n", path);
		return -1;
	} else if (ret == 0) {
		sprintf(msg + strlen(msg), "Error: Path '%s' doesn't exist.\n", path);
		return -1;
	} else if (par_inode->file_type != RD_DIRECTORY) {
		sprintf(msg + strlen(msg), "Error: Path '%s' is not a dir path.\n", path);
		return -1;
	}
	sprintf(msg + strlen(msg), "====================Directory Status====================\n");
	sprintf(msg + strlen(msg), "Directory Path: %s\n\n", path);
	sprintf(msg + strlen(msg), "InodeNum\tFilename\n");
	size_count = 0;
	max_dentry_num = RD_BLOCK_SIZE / sizeof(rd_dentry);
	for (i = 0 ; i < inode->block_count; ++i) {
		dentry = (rd_dentry*)inode->block_addr[i];
		for (j = 0; j < max_dentry_num; ++j) {
			if (dentry->inode_num != -1)
				sprintf(msg + strlen(msg), "%d\t\t%s\n", dentry->inode_num, dentry->filename);
			size_count += sizeof(rd_dentry);
			if (size_count >= inode->file_size) {
				sprintf(msg + strlen(msg), "========================================================\n");
				return 0;
			}
			dentry++;
		}
		size_count += RD_BLOCK_SIZE - (size_count % RD_BLOCK_SIZE);
	}
	return 0;

}

/*
 * Show the status of the File Descriptor Table
 */
int show_fdt_status(char *msg) {
	int i;
	rd_file *file;

	file = NULL;
	sprintf(msg + strlen(msg), "=======================FDT Status=======================\n");
	sprintf(msg + strlen(msg), "Fd\tInodeNum\tOffset\n");
	for (i = 0; i < RD_MAX_FILE; ++i) {
		if (fd_list[i] == NULL)
			continue;
		file = fd_list[i];
		sprintf(msg + strlen(msg), "%d\t%d\t\t%d\n", i, file->inode->inode_num, file->offset);
	}
	sprintf(msg + strlen(msg), "========================================================\n");
	return 0;
}