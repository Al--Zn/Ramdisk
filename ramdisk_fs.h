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
    unsigned int block_num;     /* file size (number of blocks) */
    char* block_addr[10];       /* direct block index */
    char* first_level_index;    /* 1st level index */
    char* second_level_index;   /* 2nd level index */
    char* third_level_index;    /* 3rd level index */
} rd_inode;

#define RD_DISK_SIZE        (1024 * 1024 * 2)       /* The size of the ramdisk, default 2M */
#define RD_BLOCK_SIZE       512                     /* The size of a data block, default 512 bytes */
#define RD_SUPERBLOCK_SIZE  RD_BLOCK_SIZE           /* The size of superblock, default 1 block */
#define RD_INODES_SIZE      (128 * RD_BLOCK_SIZE)   /* The size of inodes, default 128 block */
#define RD_BLOCKBITMAP_SIZE (2 * RD_BLOCK_SIZE)     /* The size of blockbitmap, default 2 block */
#define RD_DATA_BLOCKS_SIZE (RD_DISK_SIZE - RD_SUPERBLOCK_SIZE - RD_INODES_SIZE - RD_BLOCKBITMAP_SIZE)

#define RD_FILE             0xdead                  /* File Type Macro */
#define RD_DIRECTORY        0xbeef                  /* Directory Type Macro */
#define RD_AVAILABLE        0xabcd

#define RD_INODE_NUM    (RD_INODES_SIZE / (sizeof(rd_inode)))
#define RD_BLOCK_NUM    (RD_DATA_BLOCKS_SIZE / RD_BLOCK_SIZE)
#define RD_FREE 0
#define RD_ALLOCATED 1

                                                                                                                
int init_fs(void);
int init_superblock(void);
int init_inodes(void);
int init_bitmap(void);
int init_data_blocks(void);

int exit_fs(void);