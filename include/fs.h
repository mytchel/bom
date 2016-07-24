#ifndef __FS_H
#define __FS_H

enum {
	REQ_open, REQ_close,
	REQ_read, REQ_write,
	REQ_walk, REQ_create,
	REQ_remove, 
};

struct request {
	uint32_t rid;
	uint8_t type;

	uint32_t fid;
	size_t n;
	uint8_t *buf;
};

struct response {
	uint32_t rid;
	int err;
	
	size_t n;
	uint8_t *buf;
};

#define ATTR_rd		(1<<0)
#define ATTR_wr		(1<<1)
#define ATTR_dir	(1<<2)

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
