#include "ramdisk_defs.h"

typedef struct {

	int fd;							/* the request fd */	
	int mode;						/* the request mode to open file */
	char path[RD_MAX_PATH_LEN];		/* the request path */
	char data[RD_MAX_FILE_SIZE];	/* the data to write */
	int len;						/* the length to write */
	int offset;						/* the offset for lseek */	
	char *msg_addr;					/* user addr for msg */
	char *data_addr;				/* user addr for read cmd */

} rd_param;