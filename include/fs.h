#ifndef __FS_H
#define __FS_H

enum {
	REQ_open, REQ_close,
	REQ_read, REQ_write,
};

struct request {
	int fid;
	int type;

	int n;
	uint8_t *buf;
};

struct response {
	int fid;
	int err;
	
	int n;
	uint8_t *buf;
};

#endif
