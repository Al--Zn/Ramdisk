#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include "ramdisk_param.h"
#include "ramdisk_defs.h"

int fd, retfd, ret;
int command;
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

/*
 * Parse the command from user input
 * 
 * The parameters in the command are all collected in "rd_param* param"
 *
 * return 0 if the command is valid, -1 otherwise
 *
 * Input Example:
 * 	create /a.txt	
 * 	mkdir /b
 * 	open /a.txt RD_RDONLY
 * 	close 1
 * 	unlink /a.txt
 *  showblocks
 *  showinodes
 *  showdir /b
 *  showfdt
 * 	exit
 */
int parse_command(char *str, rd_param* param) {
	int cmd;	// RD_CREATE, RD_MKDIR...
	int mode;	// RD_RDONLY...
	int fd;
	int cnt;
	char* buf;
	char path[RD_MAX_PATH_LEN] = {0};
	int i, l;

	fd = 0;
	cmd = 0;
	mode = 0;
	
	for (cnt = 0, buf = strsep(&str, " "); buf != NULL; buf = strsep(&str, " ")) {
		if (strlen(buf) == 0) continue; // eat extra spaces
		switch (cnt) {
		case 0: {
			if (strcmp(buf, "create") == 0) {
				cmd = RD_CREATE;
			} else if (strcmp(buf, "mkdir") == 0) {
				cmd = RD_MKDIR;
			} else if (strcmp(buf, "open") == 0) {
				cmd = RD_OPEN;
			} else if (strcmp(buf, "close") == 0) {
				cmd = RD_CLOSE;
			} else if (strcmp(buf, "unlink") == 0) {
				cmd = RD_UNLINK;
			} else if (strcmp(buf, "showblocks") == 0) {
				cmd = RD_SHOWBLOCKS;
			} else if (strcmp(buf, "showinodes") == 0) {
				cmd = RD_SHOWINODES;
			} else if (strcmp(buf, "showdir") == 0) {
				cmd = RD_SHOWDIR;
			} else if (strcmp(buf, "showfdt") == 0) {
				cmd = RD_SHOWFDT;
			} else if (strcmp(buf, "exit") == 0 ){
				cmd = RD_EXIT;
			} else {
				cmd = RD_EXIT;
				return -1;
			}
			break;
		}
		case 1: {
			switch (cmd) {
			case RD_CREATE:
			case RD_MKDIR:
			case RD_OPEN:
			case RD_UNLINK:
			case RD_SHOWDIR:
				if (strlen(buf) > RD_MAX_PATH_LEN) {
					// too large
					return -1;
				}
				strcpy(path, buf);
				break;
			case RD_CLOSE:
				for (i = 0; l = strlen(buf); ++i) {
					if ('0' <= buf[i] && buf[i] <= '9') {
						fd = fd * 10 + (buf[i] - '0');
					}
					else {
						// fd contains characters other than #
						return -1;
					}
				}
				break;
			default:
				// other cmds don't have a 2nd argument
				return -1;
			}
			break;
		}
		case 2: {
			if (cmd == RD_OPEN) {
				if (strcmp(buf, "RD_RDONLY")) {
					mode = RD_RDONLY;
				} else if (strcmp(buf, "RD_WRONLY")) {
					mode = RD_WRONLY;
				} else if (strcmp(buf, "RD_RDWR")) {
					mode = RD_RDWR;
				} else {
					// unsupported mode;
					return -1;
				}
			} else {
				// too many arguments for other commands
				return -1;
			}
		}
		default: {
			// too many arguments
			return -1;
		}
		}
		++cnt;
    }

	strcpy(param->path, path);
	param->mode = mode;
	param->msg_addr = msg;
	param->fd = fd;
	return 0;
}

/*
 * While the user doesn't give 'exit' command,
 * keep reading the input from stdin and execute it
 */

int input_command() {
	// TODO: a loop to input command
	char str[512];
	rd_param* param;
	while (fgets(str, sizeof(str), stdin) != NULL) {
		if (parse_command(str, param) == -1) {
			// error
			break;
		}
		if (command == RD_EXIT) break;
		execute_command(fd, command, param);
	}
	return 0;
}

/* 
 * Execute the command
 * Return 0 if success, otherwise -1
 */

int execute_command(int device_fd, int cmd, rd_param *param) {
	return ioctl(device_fd, cmd, param);
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
		printf("After create %s:\n", param.path);
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
		printf("After mkdir %s:\n", param.path);
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
		printf("After open %s:\n", param.path);
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
		printf("After close %d:\n", param.fd);
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
		printf("After unlink %s:\n", param.path);
	 	show_blocks_status();
	 	show_inodes_status();
	 	show_dir_status("/");
	}



}