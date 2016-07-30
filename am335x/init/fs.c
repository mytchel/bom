#include "head.h"

struct dir_list {
	uint32_t fid;
	struct dir dir;
	struct dir_list *next;
};

struct file_list {
	struct dir *parent;
	int opened;
	struct file file;
	size_t len;
	uint8_t *buf;
	uint32_t (*write)(struct file_list *, uint8_t *, uint32_t, uint32_t, int32_t *);
	uint32_t (*read)(struct file_list *,  uint8_t *, uint32_t, uint32_t, int32_t *);
	struct file_list *next;
};

struct dir_list rootdir = {
	ROOTFID,
	{ 0, nil },
	nil
};

struct file_list rootfile = {
	nil,
	0,
	{
		ROOTFID,
		ATTR_rd|ATTR_dir,
		1,
		(uint8_t *) ""
	},
	0,
	nil,
	nil,
	nil,
	nil
};

int nfid = 1;
struct dir_list *dir_list = &rootdir;
struct file_list *file_list = &rootfile;

uint32_t
tmpread(struct file_list *fl, uint8_t *buf, uint32_t offset, uint32_t len, int32_t *err)
{
	uint8_t *bbuf;
	uint32_t i;

	if (offset >= fl->len) {
		*err = EOF;
		return 0;
	}
	
	bbuf = fl->buf + offset;
	for (i = 0; i + offset < fl->len && i < len; i++)
		buf[i] = bbuf[i];

	*err = OK;
	return i;
}

uint32_t
tmpwrite(struct file_list *fl, uint8_t *buf, uint32_t offset, uint32_t len, int32_t *err)
{
	uint8_t *bbuf;
	uint32_t i;
	
	if (offset + len >= fl->len) {
		bbuf = malloc(sizeof(uint8_t) * (offset + len));
		if (bbuf == nil) {
			*err = ENOMEM;
			return 0;
		}

		if (fl->len > 0) {
			memmove(bbuf, fl->buf, fl->len);		
			free(fl->buf);
		}
			
		fl->buf = bbuf;
		fl->len = offset + len;
	}

	bbuf = fl->buf + offset;
	for (i = 0; i + offset < fl->len && i < len; i++)
		bbuf[i + offset] = buf[i];
	
	*err = OK;
	return i;
}

uint32_t
comread(struct file_list *fl, uint8_t *buf, uint32_t offset, uint32_t len, int32_t *err)
{
	uint32_t i;
	
	for (i = 0; i < len; i++)
		buf[i] = getc();
	
	*err = OK;	
	return i;
}

uint32_t
comwrite(struct file_list *fl, uint8_t *buf, uint32_t offset, uint32_t len, int32_t *err)
{
	uint32_t i;

	for (i = 0; i < len; i++)
		putc(buf[i]);
	
	*err = OK;
	return i;
}

struct file_list *
newfile(uint32_t attr, uint8_t *name)
{
	struct file_list *fl;
	
	for (fl = file_list; fl != nil && fl->next != nil; fl = fl->next);
	if (fl == nil) {
		fl = file_list = malloc(sizeof(struct file_list));
	} else {
		fl->next = malloc(sizeof(struct file_list));
		fl = fl->next;
	}
	
	fl->next = nil;
	fl->opened = 0;
	fl->file.fid = nfid++;
	fl->file.attr = attr;
	fl->file.lname = strlen(name) + 1;
	fl->file.name = malloc(fl->file.lname);
	memmove(fl->file.name, name, fl->file.lname);
		
	if (attr & ATTR_dir) {
		fl->write = nil;
		fl->read = nil;
	} else {
		fl->write = &tmpwrite;
		fl->read = &tmpread;
	}

	fl->len = 0;
	fl->buf = nil;
		
	return fl;
}

struct file_list *
findfile(uint32_t fid)
{
	struct file_list *fl;
	for (fl = file_list; fl != nil; fl = fl->next)
		if (fl->file.fid == fid)
			break;
	
	return fl;
}

void
diraddfile(struct dir *d, struct file *f)
{
	struct file **files;
	int i;
	
	files = malloc(sizeof(struct file*) * (d->nfiles + 1));
	
	for (i = 0; i < d->nfiles; i++)
		files[i] = d->files[i];
	
	files[i] = f;
	
	if (d->nfiles > 0)
		free(d->files);
	
	d->nfiles++;
	d->files = files;
}

void
dirremovefile(struct dir *d, struct file *f)
{
	struct file **files;
	int i, j;
	
	files = malloc(sizeof(struct file*) * (d->nfiles - 1));
	
	i = j = 0;
	while (i < d->nfiles - 1) {
		if (d->files[i]->fid != f->fid) {
			files[j++] = d->files[i];
		}
		i++;
	}
	
	d->nfiles--;
	d->files = files;
}

struct dir *
finddir(uint32_t fid)
{
	struct dir_list *dl;
	
	for (dl = dir_list; dl != nil; dl = dl->next) {
		if (dl->fid == fid)
			return &dl->dir;
	}
	return nil;
}

struct dir *
adddir(uint32_t fid)
{
	struct dir_list *dl;
	
	for (dl = dir_list; dl != nil && dl->next != nil; dl = dl->next);
	
	dl->next = malloc(sizeof(struct dir_list));
	dl = dl->next;
	
	if (dl == nil) {
		return nil;
	}

	dl->next = nil;
	dl->fid = fid;
	dl->dir.nfiles = 0;
	dl->dir.files = nil;
	return &dl->dir;
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
	struct file_list *fl;
	
	fl = findfile(req->fid);
	if (fl == nil) {
		resp->ret = ENOFILE;
		return;
	}

	fl->opened++;
	resp->ret = OK;
}

void
bclose(struct request *req, struct response *resp)
{
	struct file_list *fl;
	
	fl = findfile(req->fid);
	if (fl == nil) {
		resp->ret = ENOFILE;
		return;
	}

	fl->opened--;
	resp->ret = OK;
}

void
bread(struct request *req, struct response *resp)
{
	uint32_t offset, len;
	struct file_list *fl;
	uint8_t *buf;
	
	buf = req->buf;
	memmove(&offset, buf, sizeof(uint32_t));
	buf += sizeof(uint32_t);
	memmove(&len, buf, sizeof(uint32_t));

	fl = findfile(req->fid);
	if (fl == nil) {
		resp->ret = ENOFILE;
		return;
	}
	
	resp->lbuf = len;
	resp->buf = malloc(sizeof(uint32_t) * len);
	resp->lbuf = fl->read(fl, resp->buf, offset, len, &resp->ret);
	if (resp->lbuf == 0)
		free(resp->buf);
}

void
bwrite(struct request *req, struct response *resp)
{
	uint32_t offset, len, n;
	struct file_list *fl;
	uint8_t *buf;
	
	buf = req->buf;
	memmove(&offset, buf, sizeof(uint32_t));
	buf += sizeof(uint32_t);
	memmove(&len, buf, sizeof(uint32_t));
	buf += sizeof(uint32_t);

	fl = findfile(req->fid);
	if (fl == nil) {
		resp->ret = ENOFILE;
		return;
	}

	n = fl->write(fl, buf, offset, len, &resp->ret);
	
	resp->lbuf = sizeof(uint32_t);
	resp->buf = malloc(sizeof(uint32_t));
	memmove(resp->buf, &n, sizeof(uint32_t));
}

void
bcreate(struct request *req, struct response *resp)
{
	uint32_t attr, lname;
	struct file_list *fl;
	struct dir *d;
	uint8_t *buf;
	
	buf = req->buf;
	memmove(&attr, buf, sizeof(uint32_t));
	buf += sizeof(uint32_t);
	memmove(&lname, buf, sizeof(uint32_t));
	buf += sizeof(uint32_t);
	
	d = finddir(req->fid);
	if (d == nil) {
		resp->ret = ENOFILE;
		return;
	}
	
	fl = newfile(attr, buf);

	fl->parent = d;
	diraddfile(d, &fl->file);
	
	if ((attr & ATTR_dir) == ATTR_dir) {
		adddir(fl->file.fid);
	}

	resp->ret = OK;
	resp->lbuf = sizeof(uint32_t);
	resp->buf = malloc(sizeof(uint32_t));
	memmove(resp->buf, &fl->file.fid, sizeof(uint32_t));
}

void
bremove(struct request *req, struct response *resp)
{
	struct file_list *fl, *fp;
	struct dir_list *d, *dt;
	
	fl = findfile(req->fid);
	if (fl == nil) {
		resp->ret = ENOFILE;
		return;
	}

	if (fl->opened > 0) {
		resp->ret = ERR;
		return;
	}

	for (fp = file_list; fp != nil && fp->next != fl; fp = fp->next);
	if (fp != nil) {
		fp->next = fl->next;
	} else {
		file_list = file_list->next;
	}

	for (d = dir_list; 
		d != nil && d->next != nil && d->next->fid != req->fid; 
		d = d->next);

	if (d != nil && d->next != nil) {
		dt = d->next;
		if (dt->dir.nfiles > 0) {
			resp->ret = ERR;
			return;
		}
		free(dt);
	
		d->next = d->next->next;
	}
		
	dirremovefile(fl->parent, &fl->file);

	free(fl->file.name);
	if (fl->len > 0)
		free(fl->buf);
	free(fl);
	
	resp->ret = OK;
}

void
initfs(void)
{
	struct dir *dev;
	struct file_list *fl;
	
	if (!uartinit()) {
		exit(-1);
	}
	
	/* Make /dev */
	fl = newfile(ATTR_dir|ATTR_wr|ATTR_rd, (uint8_t *) "dev");
	dev = adddir(fl->file.fid);
	diraddfile(&rootdir.dir, &fl->file);
	
	/* Make /dev/com */
	fl = newfile(ATTR_wr|ATTR_rd, (uint8_t *) "com");
	fl->write = &comwrite;
	fl->read = &comread;
	diraddfile(dev, &fl->file);
	
	/* Make /tmp */
	fl = newfile(ATTR_dir|ATTR_wr|ATTR_rd, (uint8_t *) "tmp");
	adddir(fl->file.fid);
	diraddfile(&rootdir.dir, &fl->file);
}

void
handlereq(struct request *req, struct response *resp)
{
	switch (req->type) {
	case REQ_open:
		bopen(req, resp);
		break;
	case REQ_close:
		bclose(req, resp);
		break;
	case REQ_walk:
		bwalk(req, resp);
		break;
	case REQ_read:
		bread(req, resp);
		break;
	case REQ_write:
		bwrite(req, resp);
		break;
	case REQ_remove:
		bremove(req, resp);
		break;
	case REQ_create:
		bcreate(req, resp);
		break;
	default:
		resp->ret = ENOIMPL;
		break;
	}
}

int
pmount(void)
{
	int in, out;
	int p1[2], p2[2];
	uint8_t buf[1024];
	struct request req;
	struct response resp;
	
	req.buf = buf;
	
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
		if (read(in, &req.rid, sizeof(req.rid)) != sizeof(req.rid))
			break;
		if (read(in, &req.type, sizeof(req.type)) != sizeof(req.type))
			break;
		if (read(in, &req.fid, sizeof(req.fid)) != sizeof(req.fid))
			break;
		if (read(in, &req.lbuf, sizeof(req.lbuf)) != sizeof(req.lbuf))
			break;
		
		resp.rid = req.rid;
		resp.lbuf = 0;
		resp.ret = OK;
		
		if (req.lbuf > 1024) {
			resp.ret = ENOMEM;
		} else if (req.lbuf > 0) {
			if (read(in, req.buf, sizeof(uint8_t) * req.lbuf) 
				!= sizeof(uint8_t) * req.lbuf) {
				break;
			}
		}
		
		if (resp.ret == OK) {
			handlereq(&req, &resp);
		}
		
		if (write(out, &resp.rid, sizeof(resp.rid)) != sizeof(resp.rid))
			break;
		if (write(out, &resp.ret, sizeof(resp.ret)) != sizeof(resp.ret))
			break;
		if (write(out, &resp.lbuf, sizeof(resp.lbuf)) != sizeof(resp.lbuf))
			break;
		
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
	int i;
	int fd;
	uint8_t c;
	uint8_t *str = (uint8_t *) "Hello, World\n";
	uint8_t *str2 = (uint8_t *) "How are you?\n";

	write(stdout, str, strlen(str) + 1);
	
	/* Some tests */
	fd = open("/tmp/test", O_WRONLY|O_CREATE, ATTR_wr|ATTR_rd);
	if (fd < 0) {
		return -4;
	} else {
		write(fd, str2, sizeof(uint8_t) * strlen(str2));
		close(fd);
	}
	
	for (i = 0; i < 5; i++) {
		write(stdout, str2, sizeof(uint8_t) * strlen(str2));
		sleep(500);
	}

	printf("open /tmp/test\n");
	fd = open("/tmp/test", O_RDONLY);
	if (fd < 0) {
		return -4;
	} else {
		printf("read /tmp/test\n");
		while (true) {
			i = read(fd, &c, sizeof(c));
			if (i != sizeof(c))
				break;
			printf("%i, '%c'\n", i, c);
		}
		
		close(fd);
		remove("/tmp/test");
	}
	
	return 4;
}
