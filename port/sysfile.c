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
	} else if (!(c->flags & O_RDONLY)) {
		return ELINK;
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
	} else if (!(c->flags & O_WRONLY)) {
		return ELINK;
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
	
	return 0;
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
	}

	in = fdtochan(current->fgroup, infd);
	if (in == nil) {
		return ERR;
	}
	
	lock(&current->ngroup->lock);

	kprintf("check path : '%s'\n", upath);
	path = strtopath((uint8_t *) upath);

	/* Add binding. */
	kprintf("new binding\n");
		
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

	kprintf("bound, now create proc\n");
	
	p = newproc();
	if (p == nil) {
		freebinding(bl->binding);
		kfree(bl);
		unlock(&current->ngroup->lock);
		return ERR;
	}
	
	forkfunc(p, &mountproc, (void *) bl->binding);
	
	procready(p);
	
	unlock(&current->ngroup->lock);

	return 0;
}

int
sysopen(va_list args)
{
	int err, flags, mode;
	const char *upath;
	struct chan *c;
	struct binding_list *bl;
	struct binding *best;
	size_t matches, bestmatches;
	struct path *path, *mpath, *pp;
	
	upath = va_arg(args, const char *);
	flags = va_arg(args, int);
	
	if (flags & O_CREATE) {
		mode = va_arg(args, int);
	} else {
		mode = 0;
	}
	
	path = strtopath((uint8_t *) upath);
	if (path == nil) {
		return ERR;
	}

	lock(&current->ngroup->lock);
	
	bestmatches = 0;
	best = nil;
	for (bl = current->ngroup->bindings; bl != nil; bl = bl->next) {
		matches = pathmatches(bl->binding->path, path);
		if (matches >= bestmatches) {
			bestmatches = matches;
			best = bl->binding;
		}
	}
	
	if (best == nil) {
		/* Nothing yet bound. */
		freepath(path);
		unlock(&current->ngroup->lock);
		return ERR;
	}
	
	pp = nil;
	mpath = path;
	while (mpath != nil && bestmatches > 0) {
		pp = mpath;
		mpath = mpath->next;
	}
	
	if (pp != nil) {
		pp->next = nil;
		freepath(path);
	}
	
	c = fileopen(best, mpath, flags, mode, &err);
	
	unlock(&current->ngroup->lock);
	
	if (c == nil) {
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
	
	return 0;
}
