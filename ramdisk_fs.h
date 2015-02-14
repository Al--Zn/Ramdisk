/* 
 * The ramdisk file system
 *
 * This fs is implemented as a simple unix-like file system, which partitions the disk size to 4 regions:
 * SuperBlock, Inodes, Bitmap, Freeblock.
 *
 * Disk Layout:
 * +------------+--------+-----------------------+-------------+
 * | Superblock | Inodes | Bitmap(Inode + Block) | Data Blocks |
 * +------------+--------+-----------------------+-------------+
 *
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/utsname.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/string.h>
#include <asm/unistd.h>
#include "ramdisk_defs.h"

/* Data structure of Superblock */
typedef struct {
    unsigned int block_count;
    unsigned int inode_count;
    unsigned int freeblock_count;
    unsigned int freeinode_count;
    char *first_inodes_block;
    char *first_bitmap_block;
    char *first_data_block;
} rd_superblock;

/* Data structure of Inode */
typedef struct {
    unsigned short inode_num;   /* inode number */
    unsigned short file_type;   /* file type (RD_FILE or RD_DIRECTORY) */
    unsigned int block_count;   /* file size (number of blocks) */
    unsigned int file_size;     /* file size (byte) */
    char* block_addr[RD_MAX_FILE_BLK];       /* direct block index */
} rd_inode;

/* Data structure of Dentry */
typedef struct {
    short inode_num;                /* inode number */
    char filename[RD_MAX_FILENAME];    /* file name */
} rd_dentry;

/* Data structure of File */
typedef struct {
    char path[RD_MAX_PATH_LEN];
    rd_inode *inode;
    rd_dentry *dentry;
    char *cur_block;
    int cur_offset;
    int mode;
} rd_file;

/* Init Functions */                                                                                                                
int ramfs_init(void);
int superblock_init(void);
int inodes_init(void);
int bitmap_init(void);
int data_init(void);
int fdt_init(void);
/* Exit Functions */
int ramfs_exit(void);

/* Block Operation Functions */
rd_inode* allocate_inode(void);
int allocate_fd(void);
char* allocate_block(void);
void free_inode(rd_inode *inode);
void free_fd(int fd);
void free_block(char *block);
void free_dentry(rd_dentry *dentry);

/* Path Functions */
int parse_path(const char *path, int type, rd_inode **parent_inode, rd_inode **file_inode, char *filename);

/* Dentry Functions */
int add_dentry(rd_inode *parent_inode, int inode_num, char *filename);
rd_dentry* get_dentry(const char *path);

/* Ioctl Functions */
int ramfs_create(const char *path);
int ramfs_mkdir(const char *path);
int ramfs_open(const char *path, int mode);
int ramfs_close(int fd);
int ramfs_read(int fd, void *buf, size_t count);
int ramfs_write(int fd, void *buf, size_t count);
int ramfs_lseek(int fd, int offset);
int ramfs_unlink(const char *path);

/* Test Functions*/
int show_blocks_status(char *buf);
int show_inodes_status(char *buf);
int show_dir_status(const char *path, char *buf);
int show_fdt_status(char *buf);