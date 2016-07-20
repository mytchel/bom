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
	r = chantypes[c->type]->read(c, (char *) kaddr(current, buf), n);
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
	r = chantypes[c->type]->write(c, (char *) kaddr(current, buf), n);
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
	
	/* Remove fd. */
	lock(&current->fgroup->lock);
	current->fgroup->chans[fd] = nil;
	unlock(&current->fgroup->lock);

	freechan(c);	
	
	return 0;
}

int
sysbind(va_list args)
{
	int infd, outfd;
	const char *upath;
	struct chan *in, *out;
	size_t elements;
	struct path *path;
	struct binding *b;
	
	outfd = va_arg(args, int);
	infd = va_arg(args, int);
	upath = va_arg(args, const char *);

	kprintf("check fds\n");
		
	out = fdtochan(current->fgroup, outfd);
	if (out == nil) {
		return ERR;
	}

	in = fdtochan(current->fgroup, infd);
	if (in == nil) {
		return ERR;
	}

	kprintf("check path : '%s'\n", upath);
	path = strtopath(upath);

	kprintf("check other bindings\n");
	/* Try match binding with an old binding. */
	elements = pathelements(path);
	for (b = current->ngroup->bindings; b != nil; b = b->next) {
		if (pathmatches(path, b->path) == elements) {
			freepath(path);
			break;	
		}
	}

	if (b == nil) {	
		/* Add binding. */
		kprintf("new binding\n");
		
		b = kmalloc(sizeof(struct binding));
		if (b == nil) {
			freepath(path);
			return ERR;
		}
		
		b->path = path;
		b->next = current->ngroup->bindings;
		current->ngroup->bindings = b;
	} else {
		/* Just need to replace chan for existing binding. */
		kprintf("override old binding\n");
		freechan(b->in);
		freechan(b->out);
		
		freepath(path);
	}
	
	b->in = in;
	b->out = out;

	b->in->refs++;
	b->out->refs++;
	
	kprintf("bound\n");

	return 0;
}

int
sysopen(va_list args)
{
	int err, flags, mode;
	const char *upath;
	char *strpath;
	struct path *path, *mpath, *pp;
	struct binding *b, *best;
	struct chan *c;
	size_t matches, bestmatches;
	
	upath = va_arg(args, const char *);
	flags = va_arg(args, int);
	
	kprintf("open '%s' with flags 0b%b\n", upath, flags);
	
	if (flags & O_CREATE) {
		mode = va_arg(args, int);
		kprintf("and mode = 0b%b\n", mode);
	} else {
		mode = 0;
	}
	
	path = strtopath(upath);
	if (path == nil) {
		return ERR;
	}

	strpath = pathtostr(path);	
	kprintf("path = '%s'\n", strpath);
	kfree(strpath);

	kprintf("check bindings\n");
	
	bestmatches = 0;
	best = nil;
	for (b = current->ngroup->bindings; b != nil; b = b->next) {
		kprintf("check a binding\n");
		matches = pathmatches(b->path, path);
		if (matches >= bestmatches) {
			kprintf("best so far\n");
			bestmatches = matches;
			best = b;
		}
	}
	
	if (best == nil) {
		/* Nothing yet bound. */
		freepath(path);
		return ERR;
	}
	
	kprintf("checked all bindings\n");	
	strpath = pathtostr(best->path);	
	kprintf("found mount binding at: '%s'\n", strpath);
	kfree(strpath);

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
	
	strpath = pathtostr(mpath);
	kprintf("so path from mount is: '%s'\n", strpath);
	kfree(strpath);
	
	c = fileopen(best, mpath, flags, &err);
	
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
