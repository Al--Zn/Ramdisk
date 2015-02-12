#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>

#include "ramdisk_fs.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LX & JTY");
MODULE_DESCRIPTION("A ramdisk device.");


/* On Ramdisk Module Init */
static int __init ramdisk_init(void);

/* On Ramdisk Module Exit */
static void __exit ramdisk_exit(void);

/* On Ramdisk Device Open */
int ramdisk_open(struct inode *inode, struct file *file);

/* On Ramdisk Device Release */
int ramdisk_release(struct inode *inode, struct file *file);

/* On Ramdisk Device Ioctl */
long ramdisk_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/* File Operations */
struct file_operations ramdisk_fops = {
	unlocked_ioctl: ramdisk_ioctl,
	open          : ramdisk_open,
	release       : ramdisk_release
};


static int __init ramdisk_init(void) {
	proc_create("ramdisk", 0444, NULL, &ramdisk_fops);
	ramfs_init();
	printk("Ramdisk Inited.\n");
	// TODO
	return 0;
}

static void __exit ramdisk_exit(void) {
	// TODO
	remove_proc_entry("ramdisk", NULL);
	printk("Ramdisk Exited.\n");
	return;
}

int ramdisk_open(struct inode *inode, struct file *file) {
	printk("Ramdisk Opened.\n");
	return 0;
}

int ramdisk_release(struct inode *inode, struct file *file) {
	printk("Ramdisk Released.\n");
	return 0;
}

long ramdisk_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
	//char path[128];
	//char dir[128];
	//int i, ret, j;
	switch(cmd) {
		case RD_CREATE:
			printk("Ramdisk ioctl create.\n");
			/* test creating a normal file */;
			ramfs_create("/jty.txt");
			/* test creating a existing file */
			ramfs_create("/jty.txt");
			/* test creating a file whose parent doesn't exist */
			ramfs_create("/a/b");
			/* test creating a file whose path is not valid */
			ramfs_create("/a.txt/");
			/* test creating a file under a dir other than root */
			ramfs_mkdir("/c");
			ramfs_create("/c/jty.txt");
			/* test tremendous file creation commands */
			// for (j = 0; j < 10; ++j) {
			// 	sprintf(dir, "/%d", j);
			// 	ramfs_mkdir(dir);
			// 	for (i = 0;;++i) {
			// 		sprintf(path, "/%d/%d.txt", j, i);

			// 		ret = ramfs_create(path);
			// 		if (ret == -1)
			// 			break;
			// 	}
			// }
			return 0;
		case RD_MKDIR:
			/* test make a normal dir */
			ramfs_mkdir("/a");
			/* test make a dir under a dir other than root */
			ramfs_mkdir("/a/b/");
			//printk("Ramdisk ioctl mkdir.\n");
			return 0;
		case RD_OPEN:
			// printk("Ramdisk ioctl open.\n");
			 ramfs_open("/c/jty.txt", RD_RDWR);
			 show_blocks_status();
			 show_inodes_status();
			 show_dir_status("/");
			 show_dir_status("/a");
			 show_dir_status("/a/b/");
			 show_dir_status("/c");
			 show_fdt_status();
			return 0;
		case RD_CLOSE:
			printk("Ramdisk ioctl close.\n");
			return 0;
		case RD_READ:
			printk("Ramdisk ioctl read.\n");
			return 0;
		case RD_WRITE:
			printk("Ramdisk ioctl write.\n");
			return 0;
		case RD_LSEEK:
			printk("Ramdisk ioctl lseek.\n");
			return 0;
		case RD_UNLINK:
			printk("Ramdisk ioctl unlink.\n");
			ramfs_unlink("/jty.txt");
			show_blocks_status();
			show_inodes_status();
			show_dir_status("/");
			ramfs_create("/haha.txt");
			show_blocks_status();
			show_inodes_status();
			show_dir_status("/");
			return 0;
		default:
			/* Control should never reach here =w= */
			printk("Ramdisk ioctl error.\n");
			return -1;
	}
}

module_init(ramdisk_init);
module_exit(ramdisk_exit);