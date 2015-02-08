#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LX & JTY");
MODULE_DESCRIPTION("A ramdisk device.");


 /* Device Mayjor Number */
const int ramdisk_num = 330;

/* ioctl commands */
enum {
	CREATE,
	MKDIR,
	OPEN,
	CLOSE,
	READ,
	WRITE,
	LSEEK,
	UNLINK,
	READDIR
};

/* File Operations */
struct file_operations ramdisk_fops = {
	unlocked_ioctl: ramdisk_ioctl
};

/* On Ramdisk Module Init */
static int __init ramdisk_init(void);

/* On Ramdisk Module Exit */
static int __exit ramdisk_exit(void);

/* On Ramdisk Device Open */
int ramdisk_open(struct inode *inode, struct file *file);

/* On Ramdisk Device Release */
int ramdisk_release(struct inode *inode, struct file *file);

/* On Ramdisk Device Ioctl */
long ramdisk_ioctl(struct file *file, unsigned int cmd, unsigned long arg);




static int __init ramdisk_init(void) {
	proc_entry = proc_create("ramdisk", 0444, NULL, &ramdisk_fops);
	printk("Ramdisk Init.\n");
	// TODO
	return 0;
}

static int __exit ramdisk_exit(void) {
	// TODO
	printk("Ramdisk Exit.\n");
	return 0;
}

long ramdisk_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
	switch(cmd) {
		case CREATE:
			printk("Ramdisk ioctl create.\n");
			return 0;
		case MKDIR:
			printk("Ramdisk ioctl mkdir.\n");
			return 0;
		case OPEN:
			printk("Ramdisk ioctl open.\n");
			return 0;
		case CLOSE:
			printk("Ramdisk ioctl close.\n");
			return 0;
		case READ:
			printk("Ramdisk ioctl read.\n");
			return 0;
		case WRITE:
			printk("Ramdisk ioctl write.\n");
			return 0;
		case LSEEK:
			printk("Ramdisk ioctl lseek.\n");
			return 0;
		case UNLINK:
			printk("Ramdisk ioctl unlink.\n");
			return 0;
		case READDIR:
			printk("Ramdisk ioctl readdir.\n");
			return 0;
		default:
			/* Control should never reach here =w= */
			printk("Ramdisk ioctl error.\n");
			return -1;
	}
}

