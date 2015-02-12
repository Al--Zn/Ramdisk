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
    char* block_addr[10];       /* direct block index */
    char* first_level_index;    /* 1st level index */
    char* second_level_index;   /* 2nd level index */
    char* third_level_index;    /* 3rd level index */
} rd_inode;

/* Data structure of Dentry */
typedef struct {
    unsigned short inode_num;       /* inode number */
    char filename[60]; /* file name */
} rd_dentry;

/* Data structure of File */
typedef struct {
    char path[128];
    rd_inode *inode;
    rd_dentry *dentry;
    char *cur_block;
    int cur_offset;
    int mode;
} rd_file;

/* Size Definition*/
#define RD_DISK_SIZE        (1024 * 1024 * 2)       /* The size of the ramdisk, default 2M */
#define RD_BLOCK_SIZE       512                     /* The size of a data block, default 512 bytes */
#define RD_SUPERBLOCK_SIZE  RD_BLOCK_SIZE           /* The size of superblock, default 1 block */
#define RD_INODES_SIZE      (128 * RD_BLOCK_SIZE)   /* The size of inodes, default 128 block */
#define RD_BLOCKBITMAP_SIZE (2 * RD_BLOCK_SIZE)     /* The size of blockbitmap, default 2 block */
#define RD_DATA_BLOCKS_SIZE (RD_DISK_SIZE - RD_SUPERBLOCK_SIZE - RD_INODES_SIZE - RD_BLOCKBITMAP_SIZE)
#define RD_INODE_NUM    (RD_INODES_SIZE / (sizeof(rd_inode)))
#define RD_BLOCK_NUM    (RD_DATA_BLOCKS_SIZE / RD_BLOCK_SIZE)

/* File Type Definition */
#define RD_FILE             0xdead                  
#define RD_DIRECTORY        0xbeef                  
#define RD_AVAILABLE        0xabcd
#define RD_FILEORDIR        0xdcba

/* Block Status Definition */
#define RD_FREE             0
#define RD_ALLOCATED        1

/* File Attributes Definition */

/* ioctl commands */
#define RD_CREATE           0xf1 
#define RD_MKDIR            0xf2
#define RD_OPEN             0xf3
#define RD_CLOSE            0xf4
#define RD_READ             0xf5
#define RD_WRITE            0xf6
#define RD_LSEEK            0xf7
#define RD_UNLINK           0xf8

/* File Definitions */
#define RD_MAX_FILE         256
#define RD_RDONLY           0xe1
#define RD_WRONLY           0xe2
#define RD_RDWR             0xe3

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
int show_blocks_status(void);
int show_inodes_status(void);
int show_dir_status(const char *path);
int show_fdt_status(void);