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

void
bwalk(struct request *req, struct response *resp)
{
	resp->ret = OK;
	if (req->fid == ROOTFID) {
		resp->buf = dirtowalkresponse(&root, &resp->lbuf);
	} else if (req->fid == 1) {
		resp->buf = dirtowalkresponse(&devdir, &resp->lbuf);
	} else {
		resp->ret = ENOFILE;
	}
}

void
bopen(struct request *req, struct response *resp)
{
	if (req->fid != 2) {
		resp->ret = ENOFILE;
		return;
	}

	printf("open %i\n", req->fid);	
	resp->ret = OK;
}

void
bread(struct request *req, struct response *resp)
{
	if (req->fid != 2) {
		resp->ret = ENOFILE;
		return;
	}
	
	resp->ret = ENOIMPL;
}

void
bwrite(struct request *req, struct response *resp)
{
	uint32_t offset, len;
	uint8_t *buf;

	if (req->fid != 2) {
		resp->ret = ENOFILE;
		return;
	}
	
	buf = req->buf;
	
	memmove(&offset, buf, sizeof(uint32_t));
	buf += sizeof(uint32_t);
	memmove(&len, buf, sizeof(uint32_t));
	buf += sizeof(uint32_t);
	
	printf("should write %i bytes to %i with offset %i from '%s'\n", 
		len, req->fid, offset, buf);
	
	resp->ret = len;
}

int
pmount(void)
{
	int in, out;
	int p1[2], p2[2];
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
		if (read(in, &req.rid, sizeof(uint32_t)) < 0) break;
		if (read(in, &req.type, sizeof(uint8_t)) < 0) break;
		if (read(in, &req.fid, sizeof(uint32_t)) < 0) break;
		if (read(in, &req.lbuf, sizeof(uint32_t)) < 0) break;
		
		if (req.lbuf > 0) {
			req.buf = malloc(sizeof(uint8_t) * req.lbuf);
			if (read(in, req.buf, req.lbuf) != req.lbuf) {
				break;
			}
		}
		
		resp.rid = req.rid;
		resp.lbuf = 0;
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
			resp.ret = ENOIMPL;
			break;
		}
		
		if (write(out, &resp.rid, sizeof(uint32_t)) < 0) break;
		if (write(out, &resp.ret, sizeof(int32_t)) < 0) break;
		if (write(out, &resp.lbuf, sizeof(uint32_t)) < 0) break;
		
		if (resp.lbuf > 0) {
			if (write(out, resp.buf, resp.lbuf) != resp.lbuf) {
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
	int fd, i = 0;
	char *str = "Hello\n";
	
	printf("%i : Open some random paths\n", pid);
	
	/* Some tests */
	open("../hello/there/../testing", O_RDONLY);
	open("/../dev", O_RDONLY);
	open("../com", O_RDONLY);
	open("/././.", O_RDONLY);
	open("./com", O_RDONLY);
	open("com", O_RDONLY);
	open("/dev/hello", O_RDONLY);
	open("/dev/com/shouldfail", O_RDONLY);
	
	printf("%i: open /dev/com\n", pid);

	fd = open("/dev/com", O_WRONLY);
	if (fd < 0) {
		printf("%i: open /dev/com failed with err %i\n", pid, fd);
		return -4;
	}
	
	while (i++ < 5) {
		printf("%i: write to /dev/com\n", pid);	
		write(fd, str, sizeof(char) * (1+strlen((const uint8_t *) str)));
		sleep(1000);
	}
	
	return 4;
}
