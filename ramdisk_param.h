#include "ramdisk_defs.h"

typedef struct {

	int fd;						/* the request fd */	
	int mode;					/* the request mode to open file */
	char path[RD_MAX_PATH_LEN];	/* the request path */
	char *msg_addr;				/* user addr for msg */


} rd_param;