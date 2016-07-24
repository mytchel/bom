#include "../include/types.h"
#include "../include/std.h"
#include "../include/stdarg.h"
#include "../include/fs.h"

#include "head.h"

struct file comfile = {
	2, ATTR_rd | ATTR_wr,
	4, (uint8_t *) "com"
};

struct file *devfiles[] = { &comfile };

struct dir devdir = {
	1,
	(struct file **) devfiles,
};

struct file devfile = { 
	1,	ATTR_rd | ATTR_wr | ATTR_dir,
	4, (uint8_t *) "dev"
};

struct file *rootfiles[] = { &devfile };

struct dir root = {
	1,
	(struct file **) rootfiles,
};

uint8_t *
dumpdir(struct dir *dir, size_t *size)
{
	uint8_t *buf, *b;
	struct file *f;
	int i;
	
	*size = sizeof(struct dir);
	
	for (i = 0; i < dir->nfiles; i++) {
		*size += sizeof(struct file *) + sizeof(struct file);
		*size += sizeof(uint8_t) * dir->files[i]->namelen;
	}
	
	buf = malloc(*size);
	if (buf == nil) {
		return nil;
	}
	
	b = buf;
	
	memmove(b, (const void *) dir, sizeof(struct dir));
	b += sizeof(struct dir);
	b += sizeof(struct file *) * dir->nfiles;
	
	for (i = 0; i < dir->nfiles; i++) {
		f = dir->files[i];
		memmove(b, (const void *) f, sizeof(struct file));
		b += sizeof(struct file);
		memmove(b, (const void *) f->name, sizeof(uint8_t) * f->namelen);
		b += sizeof(uint8_t) * f->namelen;
	}
	
	return buf;
}

void
bwalk(struct request *req, struct response *resp)
{
	resp->err = OK;
	if (req->fid == 0) {
		resp->buf = dumpdir(&root, &resp->n);
	} else if (req->fid == 1) {
		resp->buf = dumpdir(&devdir, &resp->n);
	} else {
		resp->err = ENOFILE;
	}
}

void
bopen(struct request *req, struct response *resp)
{
	if (req->fid != 2) {
		resp->err = ENOFILE;
		return;
	}
	
	resp->err = OK;
}

void
bread(struct request *req, struct response *resp)
{
	if (req->fid != 2) {
		resp->err = ENOFILE;
		return;
	}
	
	resp->err = ENOIMPL;
}

void
bwrite(struct request *req, struct response *resp)
{
	size_t i;
	
	if (req->fid != 2) {
		resp->err = ENOFILE;
		return;
	}
	
	resp->err = OK;
	resp->n = 0;
	
	for (i = 0; i < req->n; i++)
		putc(req->buf[i]);
}

int
pmount(void)
{
	int in, out;
	int p1[2], p2[2];
	uint8_t buf[512];
	struct request req;
	struct response resp;
	
        if (pipe(p1) == ERR) {
        	return -3;
        } else if (pipe(p2) == ERR) {
        	return -4;
        }
	
	if (bind(p1[1], p2[0], "/") < 0) {
		return -5;
	}
	
	close(p1[1]);
	close(p2[0]);
	
	in = p1[0];
	out = p2[1];

	while (true) {
		if (read(in, &req, sizeof(struct request)) < 0) {
			break;
		}
		
		if (req.n > 0) {
			req.buf = buf;
			if (read(in, req.buf, req.n) != req.n) {
				break;
			}
		}
	
		resp.n = 0;
		resp.rid = req.rid;		
		switch (req.type) {
		case REQ_open:
			bopen(&req, &resp);
			break;
		case REQ_walk:
			bwalk(&req, &resp);
			break;
		case REQ_read:
			bread(&req, &resp);
			break;
		case REQ_write:
			bwrite(&req, &resp);
			break;
		default:
			resp.err = ENOIMPL;
			break;
		}
		
		if (write(out, &resp, sizeof(struct response)) < 0) {
			break;
		}
		
		if (resp.n > 0) {
			if (write(out, resp.buf, resp.n) != resp.n) {
				break;
			}
			
			free(resp.buf);
		}
	}

	return 3;
}

int
pfile_open(void)
{
	int pid = getpid();
	int fd;
	char *str = "Hello\n";
	
	printf("%i : Open some random paths\n", pid);
	
	/* Some tests */
	open("../hello/there/../testing", O_RDONLY);
	open("/../dev", O_RDONLY);
	open("../com", O_RDONLY);
	open("/././.", O_RDONLY);
	open("./com", O_RDONLY);
	open("com", O_RDONLY);
	
	printf("%i: open /dev/com\n", pid);

	fd = open("/dev/com", O_WRONLY);
	if (fd < 0) {
		printf("%i: open /dev/com failed with err %i\n", pid, fd);
		return -4;
	}
	
	while (true) {
		printf("%i: write to /dev/com\n", pid);	
		write(fd, str, sizeof(char) * strlen((const uint8_t *) str));
		sleep(5000);
	}
	
	return 4;
}
