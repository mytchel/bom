#include "dat.h"

struct cfile {
	uint32_t fid;
	bool dir;
	struct binding *binding;
	uint32_t offset;
};

static bool
respread(struct chan *c, struct response *resp)
{
	if (piperead(c, (void *) &resp->rid, sizeof(uint32_t)) <= 0) return false;
	if (piperead(c, (void *) &resp->ret, sizeof(int32_t)) <= 0) return false;
	if (piperead(c, (void *) &resp->lbuf, sizeof(uint32_t)) <= 0) return false;

	if (resp->lbuf > 0) {
		resp->buf = malloc(sizeof(uint8_t) * resp->lbuf);
		if (resp->buf == nil) {
			printf("kproc mount : error allocating resp buf\n");
			return false;
		}

		if (piperead(c, resp->buf, resp->lbuf) != resp->lbuf) {
			printf("kproc mount : Error reading resp buf\n");
			return false;
		}
	}
	
	return true;
}

int
mountproc(void *arg)
{
	struct binding *b = arg;
	struct proc *p, *pp;
	
	while (b->refs > 0) {
		b->resp = malloc(sizeof(struct response));
		if (b->resp == nil) {
			printf("kproc mount : error allocating response.\n");
			break;
		}

		if (!respread(b->in, b->resp)) {
			printf("kproc mount: error reading responses.\n");
			break;
		}
		
		lock(&b->lock);
		
		pp = nil;
		for (p = b->waiting; p != nil; pp = p, p = p->wnext) {
			if (p->rid == b->resp->rid) {
				procready(p);
				if (pp == nil) {
					b->waiting = p->wnext;
				} else {
					pp->wnext = p->wnext;
				}
				break;
			}
		}
		
		unlock(&b->lock);

		procwait(current);
		schedule();
	}
	
	printf("No longer bound\n");

	/* Free binding and exit. */

	printf("wake waiters\n");
	
	b->resp = nil;
		
	/* Wake up any waiting processes so they can error. */
	for (p = b->waiting; p != nil; p = p->wnext) {
		procready(p);
	}
	
	printf("lock\n");
	lock(&b->lock);	

	printf("free chans\n");	
	freechan(b->in);
	freechan(b->out);
	
	printf("free path\n");
	freepath(b->path);
	
	if (b->resp) {
		printf("bind free'd with response\n");
		free(b->resp);
	}
	
	free(b);
	
	printf("return\n");
	
	return 0;
}

static struct response *
makereq(struct binding *b, struct request *req)
{
	struct response *resp;
	
	lock(&b->lock);

	req->rid = b->nreqid++;

	if (pipewrite(b->out, (void *) &req->rid, sizeof(uint32_t)) < 0)
		return nil;
	if (pipewrite(b->out, (void *) &req->type, sizeof(uint8_t)) < 0)
		return nil;
	if (pipewrite(b->out, (void *) &req->fid, sizeof(uint32_t)) < 0)
		return nil;
	if (pipewrite(b->out, (void *) &req->lbuf, sizeof(uint32_t)) < 0)
		return nil;
	
	if (req->lbuf > 0) {
		if (pipewrite(b->out, req->buf, req->lbuf) < 0) 
			return nil;
	}
	
	current->rid = req->rid;
	current->wnext = b->waiting;
	b->waiting = current;
	
	unlock(&b->lock);
	
	procwait(current);
	schedule();
	
	resp = b->resp;
	
	procready(b->srv);
	
	return resp;
}

/*
 * Find the binding that matches path to at most a depth of depth
 * return the best binding.
 */
 
static struct binding *
findbinding(struct path *path, int depth)
{
	struct binding_list *bl;
	struct path *pp, *bp;
	struct binding *best;
	int d, bestd;
	
	lock(&current->ngroup->lock);

	best = nil;
	bestd = -1;
	for (bl = current->ngroup->bindings; bl != nil; bl = bl->next) {
		d = 0;
		pp = path;
		bp = bl->binding->path;
		while (d < depth && pp != nil && bp != nil) {
			if (!strcmp(pp->s, bp->s))
				break;
			d++;
			pp = pp->next;
			bp = bp->next;
		}
		
		if (bp == nil && d > bestd) {
			bestd = d;
			best = bl->binding;
		}
	}
	
	unlock(&current->ngroup->lock);
	
	return best;
}

static struct file *
findfileindir(struct dir *dir, uint8_t *name)
{
	struct file *f;
	size_t i;
	
	for (i = 0; i < dir->nfiles; i++) {
		f = dir->files[i];
		if (strcmp(f->name, name)) {
			return f;
		}
	}
	
	return nil;
}

static struct binding *
filewalk(struct path *path, struct file *parent, struct file *file, int *err)
{
	struct binding *b, *bp;
	struct response *resp;
	struct request req;
	struct path *p;
	struct dir *dir;
	struct file *f;
	size_t depth;
	
	req.type = REQ_walk;
	req.lbuf = 0;
	req.fid = ROOTFID;
	
	b = findbinding(path, 0);
	bp = nil;
	p = path;
	depth = 0;
	*err = OK;
	while (p != nil) {
		resp = makereq(b, &req);
		if (resp == nil) {
			*err = ELINK;
			return nil;
		} else if (resp->ret != OK) {
			*err = resp->ret;
			if (resp->lbuf > 0)
				free(resp->buf);
			free(resp);
			return nil;
		} else if (resp->lbuf == 0) {
			free(resp);
			*err = ELINK;
			return nil;
		}
		
		dir = walkresponsetodir(resp->buf, resp->lbuf);
		if (dir == nil) {
			*err = ELINK;
			return nil;
		}
		
		f = findfileindir(dir, p->s);
		if (f == nil) {
			printf("'%s' not found\n", p->s);
			*err = ENOFILE;
		} else if (p->next) {
			if ((f->attr & ATTR_dir) == 0) {
				printf("'%s' not a dir\n", p->s);
				/* Part of path not a dir, so no such file. */
				*err = ENOFILE;
			} else if (p->next->next == nil) {
				printf("'%s' last dir\n", p->s);
				*err = OK;
				parent->attr = f->attr;
				parent->fid = f->fid;
			} else {
				printf("'%s' dir\n", p->s);
				*err = OK;
			}
			
			req.fid = f->fid;
		} else {
			printf("'%s' file\n", p->s);
			file->attr = f->attr;
			file->fid = f->fid;
			*err = OK;
		}
		
		free(resp->buf);
		free(resp);
		
		if (*err != OK) {
			return b;
		}
		
		p = p->next;

		bp = b;
		b = findbinding(path, ++depth);
		if (b != bp) {
			/* If we havent encountered this binding yet then it is the
			 * root of the binding. */
			req.fid = b->rootfid;
		}
	}
	
	return b;
}

static uint32_t
filecreate(struct binding *b, uint32_t pfid, uint8_t *name, 
	uint32_t cattr, int *err)
{
	struct response *resp;
	struct request req;
	uint8_t *buf;
	uint32_t cfid;
	int nlen;
	
	req.type = REQ_create;
	req.fid = pfid;
	
	nlen = strlen(name) + 1;
	req.lbuf = sizeof(uint32_t) * 2 + sizeof(uint8_t) * nlen;
	buf = req.buf = malloc(req.lbuf);
	
	memmove(buf, &cattr, sizeof(uint32_t));
	buf += sizeof(uint32_t);
	memmove(buf, &nlen, sizeof(uint32_t));
	buf += sizeof(uint32_t);
	memmove(buf, name, sizeof(uint8_t) * nlen);
	
	resp = makereq(b, &req);
	if (resp == nil) {
		*err = ELINK;
		return nil;
	} else if (resp->ret != OK) {
		*err = resp->ret;
	} else if (resp->lbuf != sizeof(uint32_t)) {
		*err = ELINK;
	} else {
		*err = OK;
		memmove(&cfid, resp->buf, sizeof(uint32_t));
	}

	if (resp->lbuf > 0)
		free(resp->buf);
	free(resp);
	
	return cfid;
		
}

struct chan *
fileopen(struct path *path, uint32_t mode, uint32_t cmode, int *err)
{
	struct file parent, file;
	struct path *p;
	struct binding *b;
	struct response *resp;
	struct request req;
	struct cfile *cfile;
	struct chan *c;
	uint32_t attr;
	
	req.type = REQ_open;
	req.lbuf = 0;
	
	b = filewalk(path, &parent, &file, err);
	if (*err != OK) {
		if (b == nil || cmode == 0) {
			return nil;
		} else {
			/* Create the file first. */
			for (p = path; p != nil && p->next != nil; p = p->next);
			if (p == nil)
				return nil;
			
			req.fid = filecreate(b, parent.fid, p->s, cmode, err);
			attr = cmode;
			if (*err != OK) {
				return nil;
			}
		}
	} else {
		req.fid = file.fid;
		attr = file.attr;
	}
	
	resp = makereq(b, &req);
	
	*err = resp->ret;

	if (resp->lbuf > 0)
		free(resp->buf);	
	free(resp);
	
	if (*err != OK) {
		return nil;
	}
	
	c = newchan(CHAN_file, mode, path);
	if (c == nil) {
		return nil;
	}
	
	cfile = malloc(sizeof(struct cfile));
	if (cfile == nil) {
		freechan(c);
		return nil;
	}
	
	c->aux = cfile;
	
	cfile->fid = req.fid;
	cfile->binding = b;
	cfile->offset = 0;
	if (attr & ATTR_dir)
		cfile->dir = true;
	else
		cfile->dir = false;

	return c;
}

int
fileread(struct chan *c, uint8_t *buf, size_t n)
{
	struct request req;
	struct request_read read;
	struct response *resp;
	struct cfile *cfile;
	int err = 0;
	
	cfile = (struct cfile *) c->aux;

	if (cfile->dir) {
		req.type = REQ_walk;
		req.fid = cfile->fid;

		resp = makereq(cfile->binding, &req);
		if (resp == nil) {
			return ELINK;
		} else if (resp->ret == OK && resp->lbuf <= cfile->offset) {
			err = EOF;
		} else if (resp->ret == OK) {
			if (resp->lbuf - cfile->offset < n)
				n = resp->lbuf - cfile->offset;
				
			memmove(buf, resp->buf + cfile->offset, n);
			cfile->offset += n;
			err = n;
		} else {
			err = resp->ret;
		}
	} else {
		req.type = REQ_read;
		req.fid = cfile->fid;
		req.buf = (uint8_t *) &read;
		req.lbuf = sizeof(struct request_read);
		
		read.offset = cfile->offset;
		read.len = n;
		
		resp = makereq(cfile->binding, &req);
		if (resp == nil) {
			return ELINK;
		} else if (resp->ret != OK) {
			err = resp->ret;
		} else if (resp->lbuf > 0) {
			n = n > resp->lbuf ? resp->lbuf : n;
			memmove(buf, resp->buf, n);
			cfile->offset += n;
			err = n;
		} else {
			err = ELINK;
		}
	}

	if (resp->lbuf > 0)
		free(resp->buf);
	free(resp);
	return err;
}

int
filewrite(struct chan *c, uint8_t *buf, size_t n)
{
	struct request req;
	struct response *resp;
	struct cfile *cfile;
	uint8_t *b;
	int err;
	
	cfile = (struct cfile *) c->aux;
	
	req.type = REQ_write;
	req.fid = cfile->fid;
	req.lbuf = sizeof(uint32_t) * 2 + sizeof(uint8_t) * n;
	req.buf = b = malloc(req.lbuf);
	
	memmove(b, &cfile->offset, sizeof(uint32_t));
	b += sizeof(uint32_t);
	memmove(b, &n, sizeof(uint32_t));
	b += sizeof(uint32_t);
	/* Should change this to write strait into the pipe. */
	memmove(b, buf, sizeof(uint8_t) * n);

	resp = makereq(cfile->binding, &req);
	free(req.buf);
	
	if (resp == nil) {
		return ELINK;
	} if (resp->ret != OK) {
		err = resp->ret;
	} else if (resp->lbuf == sizeof(uint32_t)) {
		memmove(&n, resp->buf, sizeof(uint32_t));
		cfile->offset += n;
		err = n;
	} else {
		err = ELINK;
	}

	if (resp->lbuf > 0)
		free(resp->buf);
	free(resp);
	return err;
}

int
fileclose(struct chan *c)
{
	struct cfile *cfile;
	struct request req;
	struct response *resp;
	int err;
	
	cfile = (struct cfile *) c->aux;

	req.type = REQ_close;
	req.fid = cfile->fid;
	req.lbuf = 0;

	resp = makereq(cfile->binding, &req);
	err = resp->ret;
	
	if (resp->lbuf > 0)
		free(resp->buf);
	free(resp);
	
	return err;
}

int
fileremove(struct path *path)
{
	struct binding *b;
	struct response *resp;
	struct request req;
	struct file parent, file;
	int err;
	
	printf("fileremove\n");
	
	req.type = REQ_remove;
	req.lbuf = 0;
	
	b = filewalk(path, &parent, &file, &err);
	if (err != OK) {
		return ENOFILE;
	}
	
	req.fid = file.fid;
	resp = makereq(b, &req);
	
	err = resp->ret;
	if (resp->lbuf > 0)
		free(resp->buf);
	free(resp);

	printf("response got %i\n", err);
	return err;
}

struct chantype devfile = {
	&fileread,
	&filewrite,
	&fileclose,
};
