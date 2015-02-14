#define RAMDISK_PATH        "/proc/ramdisk"

/* Size Definition */
#define RD_DISK_SIZE        (1024 * 1024 * 2)       /* The size of the ramdisk, default 2M */
#define RD_BLOCK_SIZE       512                     /* The size of a data block, default 512 bytes */
#define RD_SUPERBLOCK_SIZE  RD_BLOCK_SIZE           /* The size of superblock, default 1 block */
#define RD_INODES_SIZE      (128 * RD_BLOCK_SIZE)   /* The size of inodes, default 128 block */
#define RD_BLOCKBITMAP_SIZE (2 * RD_BLOCK_SIZE)     /* The size of blockbitmap, default 2 block */
#define RD_DATA_BLOCKS_SIZE (RD_DISK_SIZE - RD_SUPERBLOCK_SIZE - RD_INODES_SIZE - RD_BLOCKBITMAP_SIZE)
#define RD_INODE_NUM        (RD_INODES_SIZE / (sizeof(rd_inode)))
#define RD_BLOCK_NUM        (RD_DATA_BLOCKS_SIZE / RD_BLOCK_SIZE)

/* File Type Definition */
#define RD_FILE             0xdead                  
#define RD_DIRECTORY        0xbeef                  
#define RD_AVAILABLE        0xabcd
#define RD_FILEORDIR        0xdcba

/* Block Status Definition */
#define RD_FREE             0
#define RD_ALLOCATED        1

/* ioctl commands */
#define RD_CREATE           0xf1 
#define RD_MKDIR            0xf2
#define RD_OPEN             0xf3
#define RD_CLOSE            0xf4
#define RD_READ             0xf5
#define RD_WRITE            0xf6
#define RD_LSEEK            0xf7
#define RD_UNLINK           0xf8
#define RD_SHOWDIR          0xf9
#define RD_SHOWBLOCKS       0xfa
#define RD_SHOWINODES       0xfb
#define RD_SHOWFDT          0xfc
#define RD_EXIT             0xff

/* File Definitions */
#define RD_MAX_FILE         256
#define RD_MAX_FILE_BLK     10
#define RD_MAX_FILENAME     60
#define RD_RDONLY           0xe1
#define RD_WRONLY           0xe2
#define RD_RDWR             0xe3

/* Path Definitions */
#define RD_MAX_PATH_LEN     128