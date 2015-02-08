obj-m += ramdisk_module.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	insmod ramdisk_module.ko

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	rmmod ramdisk_module.ko