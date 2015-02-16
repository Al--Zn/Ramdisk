#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include "ramdisk_param.h"
#include "ramdisk_defs.h"

int dev_fd, file_fd, ret;
int cmd;
rd_param param;
int file_test = 0;

char msg[4096] = {0};
char data[RD_MAX_FILE_SIZE] = {0};
/* wrapper functions */
int rd_create(char *path) {
	strcpy(param.path, path);
	ret = ioctl(dev_fd, RD_CREATE, &param);
	return ret;
}

int rd_mkdir(char *path) {
	strcpy(param.path, path);
	ret = ioctl(dev_fd, RD_MKDIR, &param);
	return ret;
}

int rd_open(char *path, int mode) {
	strcpy(param.path, path);
	param.mode = mode;
	ret = ioctl(dev_fd, RD_OPEN, &param);
	return ret;
}

int rd_close(int fd) {
	param.fd = fd;
	ret = ioctl(dev_fd, RD_CLOSE, &param);
	return ret;
}

int rd_write(int fd, char *buf, int len) {
	param.fd = fd;
	strcpy(param.data, buf);
	param.len = len;
	ret = ioctl(dev_fd, RD_WRITE, &param);
	return ret;
}

int rd_read(int fd, char *buf, int len) {
	param.fd = fd;
	param.data_addr = buf;
	param.len = len;
	ret = ioctl(dev_fd, RD_READ, &param);
	return ret;
}

int rd_lseek(int fd, int offset) {
	param.fd = fd;
	param.offset = offset;
	ret = ioctl(dev_fd, RD_LSEEK, &param);
	return ret;
}

int rd_delete(char *path) {
	strcpy(param.path, path);
	ret = ioctl(dev_fd, RD_DELETE, &param);
	return ret;
}

void show_dir_status(char *path) {
	param.msg_addr = msg;

	printf("\033[1m\033[33m");
	if (path == NULL)
		memset(param.path, 0, sizeof(param.path));
	else
		strcpy(param.path, path);
	ioctl(dev_fd, RD_SHOWDIR, &param);
	printf("%s\n", msg);
	printf("\033[0m");		
}

void show_blocks_status() {
	param.msg_addr = msg;

	printf("\033[1m\033[33m");
	ioctl(dev_fd, RD_SHOWBLOCKS, &param);
	printf("%s\n", msg);
	printf("\033[0m");	
}

void show_inodes_status() {
	param.msg_addr = msg;

	printf("\033[1m\033[33m");
	ioctl(dev_fd, RD_SHOWINODES, &param);
	printf("%s\n", msg);
	printf("\033[0m");	

}

void show_fdt_status() {
	param.msg_addr = msg;

	printf("\033[1m\033[33m");
	ioctl(dev_fd, RD_SHOWFDT, &param);
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
 * 	delete /a.txt
 *  read 1 1024
 *  write 1 abcdefg
 *  lseek 1 0
 *  showblocks
 *  showinodes
 *  showdir /b
 *  showfdt
 * 	exit
 */
int parse_command(char *str) {
	int mode;	// RD_RDONLY...
	int fd;
	int cnt;
	int len;
	int offset;
	char* buf;
	char path[RD_MAX_PATH_LEN] = {0};
	char write_data[RD_MAX_FILE_SIZE] = {0};
	int i, l;
	int write_flag = 0;

	fd = -1;
	cmd = -1;
	mode = -1;
	offset = -1;
	len = -1;
	for (cnt = 0, buf = strsep(&str, " "); buf != NULL; buf = strsep(&str, " ")) {
		if (buf[strlen(buf)-1] == '\n')
			buf[strlen(buf)-1] = 0;
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
			} else if (strcmp(buf, "delete") == 0) {
				cmd = RD_DELETE;
			} else if (strcmp(buf, "read") == 0) {
				cmd = RD_READ;
			} else if (strcmp(buf, "write") == 0) {
				cmd = RD_WRITE;
			} else if (strcmp(buf, "lseek") == 0) {
				cmd = RD_LSEEK;
			} else if (strcmp(buf, "showblocks") == 0) {
				cmd = RD_SHOWBLOCKS;
			} else if (strcmp(buf, "showinodes") == 0) {
				cmd = RD_SHOWINODES;
			} else if (strcmp(buf, "showdir") == 0) {
				cmd = RD_SHOWDIR;
			} else if (strcmp(buf, "showfdt") == 0) {
				cmd = RD_SHOWFDT;
			} else if (strcmp(buf, "help") == 0) {
				cmd = RD_HELP;
			} else if (strcmp(buf, "exit") == 0){
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
			case RD_DELETE:
			case RD_SHOWDIR:
				if (strlen(buf) > RD_MAX_PATH_LEN) {
					// too large
					return -1;
				}
				strcpy(path, buf);
				break;
			case RD_WRITE:
				str[strlen(str)-1] = 0;
				strcpy(write_data, str);
				len = strlen(str);
				write_flag = 1;
			case RD_READ:
			case RD_LSEEK:
			case RD_CLOSE:
				fd = 0;
				for (i = 0, l = strlen(buf); i < l; ++i) {
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
				if (strcmp(buf, "RD_RDONLY") == 0) {
					mode = RD_RDONLY;
				} else if (strcmp(buf, "RD_WRONLY") == 0) {
					mode = RD_WRONLY;
				} else if (strcmp(buf, "RD_RDWR") == 0) {
					mode = RD_RDWR;
				} else {
					// unsupported mode;
					return -1;
				}
			} else if (cmd == RD_READ) {
				len = 0;
				for (i = 0, l = strlen(buf); i < l; ++i) {
					if ('0' <= buf[i] && buf[i] <= '9') {
						len = len * 10 + (buf[i] - '0');
					}
					else {
						// len contains characters other than #
						return -1;
					}
				}
			} else if (cmd == RD_LSEEK) {
				offset = 0;
				for (i = 0, l = strlen(buf); i < l; ++i) {
					if ('0' <= buf[i] && buf[i] <= '9') {
						offset = offset * 10 + (buf[i] - '0');
					}
					else {
						// offset contains characters other than #
						return -1;
					}
				}
			}
			break;
		}
		default: 
			// too many arguments
			return -1;
		}
		if (write_flag == 1) break;
		++cnt;
    }
    if (cmd == RD_CREATE || cmd == RD_MKDIR ||
		cmd == RD_OPEN || cmd == RD_DELETE ||
		cmd == RD_SHOWDIR) {
    	if (strlen(path) == 0)
    		return -1;
    }
    if (cmd == RD_OPEN && mode == -1)
    	return -1;
    if ((cmd == RD_CLOSE || cmd == RD_READ || cmd == RD_WRITE)
    	 && fd == -1)
    	return -1;
    if (cmd == RD_LSEEK && offset == -1)
    	return -1;
    if (cmd == RD_READ && len == -1)
    	return -1;
    if (cmd == RD_WRITE && strlen(write_data) == 0)
    	return -1;
	strcpy(param.path, path);
	strcpy(param.data, write_data);
	param.mode = mode;
	param.fd = fd;
	param.offset = offset;
	param.len = len;
	param.msg_addr = msg;
	param.data_addr = data;
	return 0;
}

/*
 * While the user doesn't give 'exit' command,
 * keep reading the input from stdin and execute it
 */

int input_command() {
	// TODO: a loop to input command
	char str[4096];
	if (!file_test)
		printf("\033[1m\033[33mPlease enter the command(enter 'help' to see the command list):\033[0m\n");
	while (fgets(str, sizeof(str), stdin) != NULL) {
		/* ignore comments */
		if (str[0] == '#')
			continue;
		if (parse_command(str) == -1) {
			// error
			if (!file_test)
				printf("\033[1m\033[33m");
			printf("Parse error, pls check ur command.\n");
			if (!file_test)
				printf("\033[0m");
			continue;
		}
		if (cmd == RD_EXIT) break;
		ret = execute_command();
		if (ret == -1) {
			// printf("\033[1m\033[33mSorry, ramdisk failed to execute ur command.\033[0m\n");
		}
	}
	return 0;
}

/* 
 * Execute the command
 * Return 0 if success, otherwise -1
 */

int execute_command() {
	ret = ioctl(dev_fd, cmd, &param);
	if (!file_test)
		printf("\033[1m\033[33m");
	printf("%s", msg);
	if (!file_test)
		printf("\033[0m");
	switch(cmd) {
		case RD_SHOWDIR:
		case RD_SHOWFDT:
		case RD_SHOWBLOCKS:
		case RD_SHOWINODES:
			break;
		case RD_OPEN:
			if (ret != -1) {
				if (!file_test)
					printf("\033[1m\033[33m");
				printf("Fd: %d\n", ret);
				if (!file_test)
					printf("\033[0m");
			}
			break;
		case RD_READ:
			if (ret != -1) {
				if (!file_test)
					printf("\033[1m\033[33m");
				printf("Read Data: %s\n", data);
				if (!file_test)
					printf("\033[0m");
			}
			break;
		case RD_HELP:
			if (!file_test)
				printf("\033[1m\033[33m");
			printf("Command List:\n");
			printf("create <ABSOLUTE PATH> (eg. create /a.txt)\n");
			printf("mkdir <ABSOLUTE PATH> (eg. mkdir /b)\n");
			printf("open <ABSOLUTE PATH> <RD_RDONLY|RD_WRONLY|RD_RDWR> (eg. open /a.txt RD_RDWR)\n");
			printf("close <FD> (eg. close 1)\n");
			printf("write <FD> <DATA> (eg. write 1 Hello,world)\n");
			printf("lseek <FD> <OFFSET> (eg. lseek 1 0)\n");
			printf("delete <ABSOLUTE PATH> (eg. delete /a.txt)\n");
			printf("showdir <ABSOLUTE PATH> (eg. showdir /)\n");
			printf("showblocks\n");
			printf("showinodes\n");
			printf("showfdt\n");
			if (!file_test)
				printf("\033[0m");
			break;
		default:
			break;
	}
	return ret;

}

int main(int argc, char **argv) {
	FILE *in;
	FILE *out;
	if (argc == 4 && strcmp(argv[1], "-f") == 0) {
		in = freopen(argv[2], "r", stdin);
		if (in == NULL) {
			printf("\033[1m\033[33mError: Cannot open the input file '%s'.\n\033[0m", argv[2]);
			return -1;
		}
		out = freopen(argv[3], "w", stdout);
		if (out == NULL) {
			printf("\033[1m\033[33mError: Cannot open the output file '%s'.\n\033[0m", argv[3]);
			return -1;
		}
		file_test = 1;
	} else if (argc == 2) {
		if (strcmp(argv[1], "-h") == 0) {
			printf("\033[1m\033[33mUsage:\n");
			printf("    ramdisk_test <OPTION> <INPUT> <OUTPUT>\n");
			printf("OPTION:\n");
			printf("    -c: cmd line mode, user input commands manually in terminal.\n");
			printf("    -f: file mode. For this option, <INPUT> and <OUTPUT> has to "
				   "be specified.\n");
			printf("\033[0m");
			return 0;
		} else if (strcmp(argv[1], "-c") == 0) {
			file_test = 0;
		} else {
			printf("\033[1m\033[33mInvalid Command. Use 'ramdisk_test -h' to see the options.\n\033[0m");
			return -1;
		}
	} else {
		printf("\033[1m\033[33mInvalid Command. Use 'ramdisk_test -h' to see the options.\n\033[0m");
		return -1;
	}

	dev_fd = open(RAMDISK_PATH, O_RDONLY);

	if (dev_fd < 0) {
		printf("Error: Ramdisk cannot be opened. Please make sure the module has already been installed\n");
		return -1;
	}

	input_command();

}