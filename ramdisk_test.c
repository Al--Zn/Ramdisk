#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#define RAMDISK_PATH "/proc/ramdisk"

/* ioctl commands */
#define RD_CREATE  0xf1 
#define RD_MKDIR   0xf2
#define RD_OPEN    0xf3
#define RD_CLOSE   0xf4
#define RD_READ    0xf5
#define RD_WRITE   0xf6
#define RD_LSEEK   0xf7
#define RD_UNLINK  0xf8
#define RD_READDIR 0xf9



int main(int argc, char **argv) {
	int fd;
	int ret;

	/* Test 1: Open the Ramdisk */
	printf("Test 1: Open the ramdisk......");
	fd = open(RAMDISK_PATH, O_RDONLY);

	if (fd < 0) {
		printf("Failed.\n");
		printf("Error: Ramdisk cannot be opened. Please make sure the module has already been installed\n");
		return -1;
	}
	printf("Passed.\n");

	/* Test 2: Create Command */
	printf("Test 2: Create Command......");
	ret = ioctl(fd, RD_CREATE, 0);
	if (ret < 0) {
		printf("Failed.\n");
	} else {
		printf("Passed.\n");
	}

	/* Test 3: Mkdir Command */
	printf("Test 3: Mkdir Command......");
	ret = ioctl(fd, RD_MKDIR, 0);
	if (ret < 0) {
		printf("Failed.\n");
	} else {
		printf("Passed.\n");
	}

	printf("Test 4: Open Command......");
	ret = ioctl(fd, RD_OPEN, 0);
	if (ret < 0) {
		printf("Failed.\n");
	} else {
		printf("Passed.\n");
	}

	printf("Test 5: Close Command......");
	ret = ioctl(fd, RD_CLOSE, 0);
	if (ret < 0) {
		printf("Failed.\n");
	} else {
		printf("Passed.\n");
	}

	printf("Test 6: Read Command......");
	ret = ioctl(fd, RD_READ, 0);
	if (ret < 0) {
		printf("Failed.\n");
	} else {
		printf("Passed.\n");
	}

	printf("Test 7: Write Command......");
	ret = ioctl(fd, RD_WRITE, 0);
	if (ret < 0) {
		printf("Failed.\n");
	} else {
		printf("Passed.\n");
	}

	printf("Test 8: Lseek Command......");
	ret = ioctl(fd, RD_LSEEK, 0);
	if (ret < 0) {
		printf("Failed.\n");
	} else {
		printf("Passed.\n");
	}

	printf("Test 9: Unlink Command......");
	ret = ioctl(fd, RD_UNLINK, 0);
	if (ret < 0) {
		printf("Failed.\n");
	} else {
		printf("Passed.\n");
	}

	printf("Test 10: Readdir Command......");
	ret = ioctl(fd, RD_READDIR, 0);
	if (ret < 0) {
		printf("Failed.\n");
	} else {
		printf("Passed.\n");
	}
	// TODO

}