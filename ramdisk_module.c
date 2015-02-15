#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include "ramdisk_param.h"
#include "ramdisk_fs.h"
#include "ramdisk_defs.h"

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

	rd_param param;
	int fd, ret;
	char *msg, *buf;
	if (arg != 0)
		copy_from_user(&param, (rd_param*)arg, sizeof(rd_param));
	fd = -1;
	ret = 0;
	msg = (char*)vmalloc(1024);
	memset(msg, 0, 1024);
	switch(cmd) {
		case RD_CREATE:
			ret = ramfs_create(param.path, msg);
			break;
		case RD_MKDIR:
			ret = ramfs_mkdir(param.path, msg);
			break;
		case RD_OPEN:
			ret = ramfs_open(param.path, param.mode, msg);
			// copy_to_user(param.fd_addr, &fd, sizeof(int));
			break;
		case RD_CLOSE:
			ret = ramfs_close(param.fd, msg);
			break;
		case RD_READ:
			buf = (char*)vmalloc(param.len);
			memset(buf, 0, param.len);
			ret = ramfs_read(param.fd, buf, param.len, msg);
			if (ret != -1)
				copy_to_user(param.data_addr, buf, param.len);
			vfree(buf);
			break;
		case RD_WRITE:
			ret = ramfs_write(param.fd, param.data, param.len, msg);
			break;
		case RD_LSEEK:
			ret = ramfs_lseek(param.fd, param.offset, msg);
			break;
		case RD_UNLINK:
			ret = ramfs_unlink(param.path, msg);
			break;
		case RD_SHOWDIR:
			show_dir_status(param.path, msg);
			break;
		case RD_SHOWBLOCKS:
			show_blocks_status(msg);
			break;
		case RD_SHOWINODES:
			show_inodes_status(msg);
			break;
		case RD_SHOWFDT:
			show_fdt_status(msg);
			break;
		case RD_HELP:
			break;
		default:
			/* Control should never reach here =w= */
			printk("Ramdisk ioctl error.\n");
			ret = -1;
			break;
	}
	copy_to_user(param.msg_addr, msg, strlen(msg) + 1);
	vfree(msg);
	return ret;
}

module_init(ramdisk_init);
module_exit(ramdisk_exit);