#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include "ramdisk_param.h"
#include "ramdisk_defs.h"

int fd, retfd, ret;
int command;
void parse_command();
rd_param param;

char msg[1024] = {0};

void show_dir_status(char *path) {
	param.msg_addr = msg;

	printf("\033[1m\033[33m");
	if (path == NULL)
		memset(param.path, 0, sizeof(param.path));
	else
		strcpy(param.path, path);
	ioctl(fd, RD_SHOWDIR, &param);
	printf("%s\n", msg);
	printf("\033[0m");		
}

void show_blocks_status() {
	param.msg_addr = msg;

	printf("\033[1m\033[33m");
	ioctl(fd, RD_SHOWBLOCKS, &param);
	printf("%s\n", msg);
	printf("\033[0m");	
}

void show_inodes_status() {
	param.msg_addr = msg;

	printf("\033[1m\033[33m");
	ioctl(fd, RD_SHOWINODES, &param);
	printf("%s\n", msg);
	printf("\033[0m");	

}

void show_fdt_status() {
	param.msg_addr = msg;

	printf("\033[1m\033[33m");
	ioctl(fd, RD_SHOWFDT, &param);
	printf("%s\n", msg);
	printf("\033[0m");	
}

int main(int argc, char **argv) {


	/* Test 1: Open the Ramdisk */
	printf("Test 1: Open the ramdisk......");
	fd = open(RAMDISK_PATH, O_RDONLY);

	if (fd < 0) {
		printf("Failed.\n");
		printf("Error: Ramdisk cannot be opened. Please make sure the module has already been installed\n");
		return -1;
	}
	printf("Succeeded.\n");


	/* Test 2: Create Command */
	printf("Test 2: Create Command......");

	strcpy(param.path, "/jty.txt");
	ret = ioctl(fd, RD_CREATE, &param);

	if (ret < 0) {
		printf("Failed.\n");
	} else {
		printf("Succeeded.\n");
		show_blocks_status();
		show_inodes_status();
		show_dir_status("/");
	}

	printf("Test 3: Mkdir Command......");
	strcpy(param.path, "/abc");
	ret = ioctl(fd, RD_MKDIR, &param);
	if (ret < 0) {
		printf("Failed.\n");
	} else {
		printf("Succeeded.\n");
		show_blocks_status();
		show_inodes_status();
		show_dir_status("/");
	}

	printf("Test 4: Open Command......");
	strcpy(param.path, "/jty.txt");
	param.mode = RD_RDWR;
	ret = ioctl(fd, RD_OPEN, &param);
	if (ret < 0) {
		printf("Failed.\n");
	} else {
		printf("Succeeded.\n");
		retfd = ret;
		show_fdt_status();
	}

	printf("Test 5: Close Command......");
	param.fd = retfd;
	ret = ioctl(fd, RD_CLOSE, &param);
	if (ret < 0) {
		printf("Failed.\n");
	} else {
		printf("Succeeded.\n");
		show_fdt_status();
	}

	// printf("Test 6: Read Command......");
	// ret = ioctl(fd, RD_READ, 0);
	// if (ret < 0) {
	// 	printf("Failed.\n");
	// } else {
	// 	printf("Succeeded.\n");
	// }

	// printf("Test 7: Write Command......");
	// ret = ioctl(fd, RD_WRITE, 0);
	// if (ret < 0) {
	// 	printf("Failed.\n");
	// } else {
	// 	printf("Succeeded.\n");
	// }

	// printf("Test 8: Lseek Command......");
	// ret = ioctl(fd, RD_LSEEK, 0);
	// if (ret < 0) {
	// 	printf("Failed.\n");
	// } else {
	// 	printf("Succeeded.\n");
	// }

	printf("Test 9: Unlink Command......");
	strcpy(param.path, "/jty.txt");
	ret = ioctl(fd, RD_UNLINK, &param);
	if (ret < 0) {
		printf("Failed.\n");
	} else {
	 	printf("Succeeded.\n");
	 	show_blocks_status();
	 	show_inodes_status();
	 	show_dir_status("/");
	}



}