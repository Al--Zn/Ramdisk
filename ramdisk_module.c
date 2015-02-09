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
	switch(cmd) {
		case RD_CREATE:
			printk("Ramdisk ioctl create.\n");
			return 0;
		case RD_MKDIR:
			printk("Ramdisk ioctl mkdir.\n");
			return 0;
		case RD_OPEN:
			printk("Ramdisk ioctl open.\n");
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
			return 0;
		default:
			/* Control should never reach here =w= */
			printk("Ramdisk ioctl error.\n");
			return -1;
	}
}

module_init(ramdisk_init);
module_exit(ramdisk_exit);