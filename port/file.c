#include "dat.h"

struct cfile {
	uint32_t fid;
	struct binding *binding;
	uint32_t offset;
};

static struct dir *
responseprocessdir(struct response *resp)
{
	struct dir *d;
	int i;
	uint8_t *buf;
	
	buf = resp->buf;
	
	d = (struct dir *) buf;
	
	buf += sizeof(struct dir);
	d->files = (struct file **) buf;
	buf += sizeof(struct file *) * d->nfiles; 

	for (i = 0; i < d->nfiles; i++) {
		d->files[i] = (struct file *) buf;
		buf += sizeof(struct file);
		d->files[i]->name = buf;
		buf += sizeof(uint8_t) * d->files[i]->namelen;
	}
	
	return d;
}

static bool
respread(struct chan *c, struct response *resp)
{
	if (piperead(c, (uint8_t *) resp, sizeof(struct response)) <= 0) {
		kprintf("kproc mount : Error reading pipe\n");
		return false;
	}

	if (resp->n > 0) {
		resp->buf = kmalloc(resp->n);
		if (resp->buf == nil) {
			kprintf("kproc mount : error allocating resp buf\n");
			return false;
		}
			
		if (piperead(c, resp->buf, resp->n) != resp->n) {
			kprintf("kproc mount : Error reading resp buf\n");
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
		b->resp = kmalloc(sizeof(struct response));
		if (b->resp == nil) {
			kprintf("kproc mount : error allocating response.\n");
			break;
		}

		if (!respread(b->in, b->resp)) {
			kprintf("kproc mount: error reading responses.\n");
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
	
	kprintf("No longer bound\n");

	/* Free binding and exit. */

	kprintf("wake waiters\n");
	
	b->resp = nil;
		
	/* Wake up any waiting processes so they can error. */
	for (p = b->waiting; p != nil; p = p->wnext) {
		procready(p);
	}
	
	kprintf("lock\n");
	lock(&b->lock);	

	kprintf("free chans\n");	
	freechan(b->in);
	freechan(b->out);
	
	kprintf("free path\n");
	freepath(b->path);
	
	if (b->resp) {
		kprintf("bind free'd with response\n");
		kfree(b->resp);
	}
	
	kfree(b);
	
	kprintf("return\n");
	
	return 0;
}

static struct response *
makereq(struct binding *b, struct request *req)
{
	struct response *resp;
	
	lock(&b->lock);

	req->rid = b->nreqid++;
	pipewrite(b->out, (uint8_t *) req, sizeof(struct request));
	
	if (req->n > 0) {
		pipewrite(b->out, req->buf, req->n);
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
 * return the best binding and set mpath to the remainder of the path.
 */
 
static struct binding *
findbinding(struct path *path, int depth, struct path **mpath)
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
			*mpath = pp;
		}
	}
	
	unlock(&current->ngroup->lock);
	
	return best;
}

bool
checkattr(uint32_t attr, uint32_t mode)
{
	bool r, w;
	r = attr & ATTR_rd;
	w = attr & ATTR_wr;
	
	if (!r && (mode & O_RDONLY)) {
		return false;
	} else if (!w && (mode & O_WRONLY)) {
		return false;
	} else {
		return true;
	}
}

struct chan *
fileopen(struct path *path, int flag, int mode, int *err)
{
	struct binding *b, *bp;
	struct path *p, *mpath;
	struct response *resp;
	struct request req;
	struct dir *dir;
	struct chan *c;
	struct cfile *cfile;
	struct file *f;
	int i, depth;
	bool found;
	
	uint8_t *str;

	str = pathtostr(path, nil);
	kprintf("trying to open file '%s'\n", str);
	kfree(str);
	
	req.type = REQ_walk;
	req.fid = 0;
	req.n = 0;
	
	b = findbinding(nil, 0, &mpath);
	if (b == nil) {
		kprintf("Root not bound!\n");
		*err = ENOFILE;
		return nil;
	}

	mpath = nil;
	bp = nil;
	p = path;
	depth = 0;
	*err = OK;
	while (p != nil) {
		b = findbinding(path, depth, &mpath);

		if (b != bp) {
			/* If we havent encountered this yet then it is the
			 * root of the binding. */
			req.fid = 0;
		}
		
		resp = makereq(b, &req);
		if (resp == nil) {
			kprintf("An error occured with the request\n");
			*err = ELINK;
			return nil;
		} else if (resp->err != OK) {
			*err = resp->err;
			kfree(resp);
			return nil;
		}
		
		dir = responseprocessdir(resp);

		found = false;
		for (i = 0; i < dir->nfiles; i++) {
			f = dir->files[i];
			
			if (strcmp(f->name, p->s)) {
				if (p->next && (f->attr & ATTR_dir) == 0) {
					*err = ENOFILE;
				} else if (!checkattr(f->attr, flag)) {
					*err = EMODE;
				} else {
					req.fid = f->fid;
					found = true;
				}
				break;
			}
		}
		
		if (resp->n > 0)
			kfree(resp->buf);
		kfree(resp);
		
		if (!found) {
			*err = ENOFILE;
			return nil;
		} else if (*err != OK) {
			return nil;
		}
		
		bp = b;
		p = p->next;
	}
	
	kprintf("file found, now open %i\n", req.fid);

	req.type = REQ_open;
	
	resp = makereq(b, &req);
	kprintf("got response, err = %i\n", resp->err);
	
	*err = resp->err;
	
	if (resp->n > 0)
		kfree(resp->buf);
	kfree(resp);
	
	if (*err != OK) {
		return nil;
	}
	
	kprintf("make a chan\n");
	c = newchan(CHAN_file, mode, path);
	if (c == nil) {
		return nil;
	}
	
	kprintf("make cfile struct\n");
	cfile = kmalloc(sizeof(struct cfile));
	if (cfile == nil) {
		freechan(c);
		return nil;
	}
	
	kprintf("set up and return new channel\n");
	c->aux = cfile;
	
	cfile->fid = req.fid;
	cfile->binding = b;
	cfile->offset = 0;

	return c;
}

int
fileread(struct chan *c, uint8_t *buf, size_t n)
{
	return ERR;
}

int
filewrite(struct chan *c, uint8_t *buf, size_t n)
{
	return ERR;
}

int
fileclose(struct chan *c)
{
	return ERR;
}

struct chantype devfile = {
	&fileread,
	&filewrite,
	&fileclose,
};
