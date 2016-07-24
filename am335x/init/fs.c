#include "../include/types.h"
#include "../include/std.h"
#include "../include/stdarg.h"
#include "../include/fs.h"

#include "head.h"

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
walk(struct request *req, struct response *resp)
{
	struct dir dir;
	struct file *files[2], f1, f2;
	
	dir.nfiles = 2;
	dir.files = (struct file **) files;
	
	files[0] = &f1;
	files[1] = &f2;
	
	f1.fid = 1;
	f1.attr = ATTR_rd | ATTR_wr | ATTR_dir;
	f1.namelen = 4;
	f1.name = (uint8_t *) "dev";
	
	f2.fid = 2;
	f2.attr = ATTR_rd | ATTR_wr;
	f2.namelen = 4;
	f2.name = (uint8_t *) "com";
	
	resp->buf = dumpdir(&dir, &resp->n);
	resp->err = OK;
}

int
pmount(void)
{
	int pid = getpid();
	int in, out;
	int p1[2], p2[2];
	uint8_t buf[512];
	struct request req;
	struct response resp;
	
        if (pipe(p1) == ERR) {
        	printf("%i: p1 pipe failed\n", pid);
        	return -3;
        } else if (pipe(p2) == ERR) {
        	printf("%i: p2 pipe failed\n", pid);
        	return -3;
        }
	
	if (bind(p1[1], p2[0], "/") < 0) {
		printf("%i: bind failed\n", pid);
		return -3;
	}
	
	sleep(1000);
	
	close(p1[1]);
	close(p2[0]);
	
	in = p1[0];
	out = p2[1];

	while (true) {
		if (read(in, &req, sizeof(struct request)) < 0) {
			printf("%i: failed to read in\n", pid);
			break;
		}
		
		printf("%i : Got request with rid: %i\n", pid, req.rid);
		
		if (req.n > 0) {
			req.buf = buf;
			if (read(in, req.buf, req.n) != req.n) {
				printf("%i: failed to read buf\n", pid);
				break;
			}
		}
	
		resp.n = 0;
		resp.rid = req.rid;		
		switch (req.type) {
		case REQ_open:
			printf("%i : should open file %i\n", pid, req.fid);
			resp.err = ENOIMPL;
			break;
		case REQ_walk:
			printf("%i : doing fs walk for fid %i\n", pid, req.fid);
			walk(&req, &resp);
			break;
		case REQ_read:
			printf("%i : should read %i\n", req.fid);
			resp.err = ENOIMPL;
			break;
		case REQ_write:
			printf("%i : should write %i\n", req.fid);
			resp.err = ENOIMPL;
			break;
		default:
			printf("%i : bad request %i\n", req.rid);
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
		printf("%i: open /com failed with err %i\n", pid, fd);
		return -4;
	}
	
	printf("%i: write to /dev/com\n", pid);	
	while (true) {
		write(fd, "Hello\n", sizeof(char) * 7);
		sleep(5000);
	}
	
	return 4;
}
