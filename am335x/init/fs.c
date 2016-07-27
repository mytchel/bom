#include "head.h"

#define FDEV 1
#define FCOM 2

struct file devfile = { 
	1, ATTR_rd | ATTR_wr | ATTR_dir,
	4, (uint8_t *) "dev"
};

struct file comfile = {
	2, ATTR_rd | ATTR_wr,
	4, (uint8_t *) "com"
};

struct dir_list {
	uint32_t fid;
	struct dir dir;
	struct dir_list *next;
};

int nfid = 3;
struct dir_list *dir_list;

struct file *
newfile(uint32_t attr, uint8_t *name)
{
	struct file *f;
	
	f = malloc(sizeof(struct file));
	f->fid = nfid++;
	f->attr = attr;
	f->lname = strlen(name);
	f->name = name;
	
	return f;
}

void
diraddfile(struct dir *d, struct file *f)
{
	struct file **files;
	int i;
	
	files = malloc(sizeof(struct file*) * (d->nfiles+1));
	
	for (i = 0; i < d->nfiles; i++)
		files[i] = d->files[i];
	
	if (d->nfiles > 0)
		free(d->files);
	
	d->nfiles++;
	d->files = files;
	
	files[i] = f; 
}

struct dir *
finddir(uint32_t fid)
{
	struct dir_list *dl;
	for (dl = dir_list; dl != nil; dl = dl->next)
		if (dl->fid == fid)
			return &dl->dir;
	return nil;
}

struct dir *
adddir(uint32_t fid)
{
	struct dir_list *dl;
	
	for (dl = dir_list; dl != nil && dl->next != nil; dl = dl->next);
	
	if (dl == nil) {
		dl = dir_list = malloc(sizeof(struct dir_list));
	} else {
		dl->next = malloc(sizeof(struct dir_list));
		dl = dl->next;
	}
	
	dl->next = nil;
	dl->fid = fid;
	dl->dir.nfiles = 0;
	dl->dir.files = nil;
	return &dl->dir;
}

void
initfs(void)
{
	struct dir *d;
	
	d = adddir(0);
	diraddfile(d, &devfile);
	d = adddir(1);
	diraddfile(d, &comfile);
}

void
bwalk(struct request *req, struct response *resp)
{
	struct dir *d;
	
	d = finddir(req->fid);
	if (d == nil) {
		resp->ret = ENOFILE;
	} else {
		resp->ret = OK;
		resp->buf = dirtowalkresponse(d, &resp->lbuf);
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
	uint32_t offset, len, i;
	uint8_t *buf;

	buf = req->buf;
	memmove(&offset, buf, sizeof(uint32_t));
	buf += sizeof(uint32_t);
	memmove(&len, buf, sizeof(uint32_t));
	buf += sizeof(uint32_t);

	i = 0;
	switch (req->fid){
	case 2:	
		while (i < len)
			putc(buf[i++]);
		break;
	default:
		resp->ret = ENOFILE;
		return;
	}
	
	resp->ret = i;
}

void
bcreate(struct request *req, struct response *resp)
{
	uint32_t attr, lname;
	uint8_t *buf, *name;
	struct file *f;
	struct dir *d;
	
	printf("bcreate\n");

	buf = req->buf;
	memmove(&attr, buf, sizeof(uint32_t));
	buf += sizeof(uint32_t);
	memmove(&lname, buf, sizeof(uint32_t));
	buf += sizeof(uint32_t);
	
	name = malloc(sizeof(uint8_t) * lname);
	memmove(name, buf, lname);
	
	d = finddir(req->fid);
	if (d == nil) {
		resp->ret = ENOFILE;
		return;
	}
	
	f = newfile(attr, name);

	printf("new file in %i, '%s' has fid %i\n", req->fid, f->name, f->fid);

	diraddfile(d, f);
	
	if (attr & ATTR_dir) {
		adddir(f->fid);
	}

	resp->ret = f->fid;
}

void
bremove(struct request *req, struct response *resp)
{
	resp->ret = ENOIMPL;
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
	
	initfs();

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
		case REQ_remove:
			bremove(&req, &resp);
			break;
		case REQ_create:
			bcreate(&req, &resp);
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
	open("/dev/hello", O_WRONLY|O_CREATE, ATTR_dir|ATTR_wr);
	
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
