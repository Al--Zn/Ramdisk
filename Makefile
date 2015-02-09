obj-m := ramdisk.o
ramdisk-objs := ramdisk_fs.o ramdisk_module.o 

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	insmod ramdisk.ko
	gcc -o ramdisk_test ramdisk_test.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	rmmod ramdisk.ko
	rm ramdisk_test