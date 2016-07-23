#ifndef __FS_H
#define __FS_H

enum {
	REQ_open, REQ_close,
	REQ_read, REQ_write,
};

struct request {
	uint32_t rid;
	uint8_t type;

	size_t n;
	uint8_t *buf;
};

struct response {
	uint32_t rid;
	uint8_t err;
	
	size_t n;
	uint8_t *buf;
};

struct file {
	uint32_t fid;
	uint32_t attr;
	
	size_t namelen;
	uint8_t *name;
};

struct dir {
	size_t nfiles;
	struct file **files;
};

#endif
