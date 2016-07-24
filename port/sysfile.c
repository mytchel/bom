#include "dat.h"

int
sysread(va_list args)
{
	int r, fd;
	size_t n;
	void *buf;
	struct chan *c;

	fd = va_arg(args, int);
	buf = va_arg(args, void *);
	n = va_arg(args, size_t);
	
	c = fdtochan(current->fgroup, fd);
	if (c == nil) {
		return ERR;
	} else if (!(c->mode & O_RDONLY)) {
		return EMODE;
	}
		
	lock(&c->lock);
	r = chantypes[c->type]->read(c, (uint8_t *) kaddr(current, buf), n);
	unlock(&c->lock);
	
	return r;
}

int
syswrite(va_list args)
{
	int r, fd;
	size_t n;
	void *buf;
	struct chan *c;

	fd = va_arg(args, int);
	buf = va_arg(args, void *);
	n = va_arg(args, size_t);
	
	c = fdtochan(current->fgroup, fd);
	if (c == nil) {
		return ERR;
	} else if (!(c->mode & O_WRONLY)) {
		return EMODE;
	}

	lock(&c->lock);
	r = chantypes[c->type]->write(c, (uint8_t *) kaddr(current, buf), n);
	unlock(&c->lock);
	
	return r;
}

int
sysclose(va_list args)
{
	int fd;
	struct chan *c;
	
	fd = va_arg(args, int);

	c = fdtochan(current->fgroup, fd);
	if (c == nil) {
		return ERR;
	}
	
	lock(&current->fgroup->lock);

	/* Remove fd. */
	current->fgroup->chans[fd] = nil;
	freechan(c);	

	unlock(&current->fgroup->lock);
	
	return OK;
}

int
sysbind(va_list args)
{
	int infd, outfd;
	const char *upath;
	struct proc *p;
	struct path *path;
	struct chan *in, *out;
	struct binding_list *bl;
	
	outfd = va_arg(args, int);
	infd = va_arg(args, int);
	upath = va_arg(args, const char *);

	out = fdtochan(current->fgroup, outfd);
	if (out == nil) {
		return ERR;
	} else if (!(out->mode & O_WRONLY)) {
		return EMODE;
	}

	in = fdtochan(current->fgroup, infd);
	if (in == nil) {
		return ERR;
	} else if  (!(in->mode & O_RDONLY)) {
		return EMODE;
	}
	
	lock(&current->ngroup->lock);

	path = realpath(current->dot, (uint8_t *) upath);

	bl = kmalloc(sizeof(struct binding_list));
	if (bl == nil) {
		freepath(path);
		unlock(&current->ngroup->lock);
		return ERR;
	}
		
	bl->binding = newbinding(path, out, in);
	if (bl->binding == nil) {
		kfree(bl);
		freepath(path);
		unlock(&current->ngroup->lock);
		return ERR;
	}
		
	bl->next = current->ngroup->bindings;
	current->ngroup->bindings = bl;

	p = newproc();
	if (p == nil) {
		freebinding(bl->binding);
		kfree(bl);
		unlock(&current->ngroup->lock);
		return ERR;
	}
	
	forkfunc(p, &mountproc, (void *) bl->binding);
	
	bl->binding->srv = p;
	
	procready(p);
	
	unlock(&current->ngroup->lock);

	return OK;
}

int
sysopen(va_list args)
{
	int err, mode, cmode;
	const char *upath;
	struct chan *c;
	struct path *path;
	
	upath = va_arg(args, const char *);
	mode = va_arg(args, int);
	
	if (mode & O_CREATE) {
		cmode = va_arg(args, int);
	} else {
		cmode = 0;
	}

	path = realpath(current->dot, (uint8_t *) upath);

	c = fileopen(path, mode, cmode, &err);
	
	if (c == nil) {
		freepath(path);
		return err;
	} else {
		return addchan(current->fgroup, c);
	}
}

int
syspipe(va_list args)
{
	int *fds;
	struct chan *c0, *c1;
	
	fds = va_arg(args, int*);

	if (!newpipe(&c0, &c1)) {
		return ERR;
	}
	
	fds[0] = addchan(current->fgroup, c0);
	fds[1] = addchan(current->fgroup, c1);
	
	return OK;
}
