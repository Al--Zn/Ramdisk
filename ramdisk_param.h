typedef struct {

	int fd;				/* the request fd */	
	int mode;				/* the request mode to open file */
	char path[256];				/* the request path */
	char *msg_addr;			/* user addr for msg */


} rd_param;