#include "dat.h"

int
syspipe(va_list args)
{
	int *fds;
	struct pipe *p0, *p1;
	
	fds = va_arg(args, int*);

	p0 = newpipe();
	if (p0 == nil) {
		return ERR;
	}
	
	p1 = newpipe();
	if (p1 == nil) {
		kfree(p0);
		return ERR;
	}
	
	p0->link = p1;
	p1->link = p0;
	
	p0->path = p1->path = nil;
	
	fds[0] = addpipe(current->fgroup, p0);
	fds[1] = addpipe(current->fgroup, p1);
	
	return 0;
}

int
sysread(va_list args)
{
	int fd;
	size_t n;
	void *buf;
	struct pipe *pipe;

	fd = va_arg(args, int);
	buf = va_arg(args, void *);
	n = va_arg(args, size_t);
	
	pipe = fdtopipe(current->fgroup, fd);
	if (pipe == nil) {
		return ERR;
	} else if (pipe->link == nil) {
		return ELINK;
	} else if (pipe->link->action == PIPE_reading) {
		/* Can not read from both ends. */
		
		return ELINK;
	}
	
	while (pipe->action != PIPE_none)
		schedule();
	
	pipe->action = PIPE_reading;
	pipe->user = current;
	pipe->buf = (uint8_t *) kaddr(current, buf);
	pipe->n = n;
	
	while (pipe->n > 0) {
		if (pipe->link->action == PIPE_writing) {
			procready(pipe->link->user);
		}
		
		procwait(current);
		schedule();
	}
	
	return n;
}

int
syswrite(va_list args)
{
	int fd;
	size_t n, i, tot;
	void *buf;
	uint8_t *in;
	struct pipe *pipe;

	fd = va_arg(args, int);
	buf = va_arg(args, void *);
	n = va_arg(args, size_t);
	
	pipe = fdtopipe(current->fgroup, fd);
	if (pipe == nil) {
		return ERR;
	} else if (pipe->link == nil) {
		return ELINK;
	} else if (pipe->link->action == PIPE_writing) {
		/* Can not write from both ends. */
		return ELINK;
	}

	while (pipe->action != PIPE_none)
		schedule();

	pipe->action = PIPE_writing;
	pipe->user = current;
	
	tot = 0;
	in = (uint8_t *) buf;
	while (tot < n) {
		if (pipe->link->action != PIPE_reading) {
			procwait(current);
			schedule();
		}
		
		for (i = 0; tot + i < n && i < pipe->link->n; i++) {
			pipe->link->buf[i] = in[i];
		}
		
		tot += i;
		in += i;
		pipe->link->buf += i;
		pipe->link->n -= i;
		
		if (pipe->link->n == 0)
			pipe->link->action = PIPE_none;
		procready(pipe->link->user);
	}
	
	pipe->action = PIPE_none;
	
	return n;
}

int
sysclose(va_list args)
{
	int fd;
	struct pipe *pipe;
	
	fd = va_arg(args, int);
	
	pipe = fdtopipe(current->fgroup, fd);
	if (pipe == nil) {
		return ERR;
	}
	
	/* Remove fd. */
	current->fgroup->pipes[fd] = nil;
	freepipe(pipe);	

	return 0;
}

int
sysbind(va_list args)
{
	int fd, flags;
	size_t elements;
	const char *upath;
	struct path *path;
	struct pipe *pipe;
	struct binding *b;
	
	fd = va_arg(args, int);
	upath = va_arg(args, const char *);
	flags = va_arg(args, int);
	
	pipe = fdtopipe(current->fgroup, fd);
	if (pipe == nil) {
		return ERR;
	}

	path = strtopath(upath);
	if (path == nil) {
		return ERR;
	}

	/* Try match binding with an old binding. */
	elements = pathelements(path);
	for (b = current->ngroup->bindings; b != nil; b = b->next) {
		if (pathmatches(path, b->path) == elements) {
			freepath(path);
			path = b->path;
			break;	
		}
	}

	if (b == nil) {	
		b = kmalloc(sizeof(struct binding));
		if (b == nil) {
			freepath(path);
			return ERR;
		}
		b->path = path;
		b->next = current->ngroup->bindings;
		current->ngroup->bindings = b;
	} else {
		freepipe(b->pipe);
	}

	b->pipe = pipe;

	return 0;
}

int
sysopen(va_list args)
{
	int flags, mode;
	const char *upath;
	struct path *path, *mpath, *pp;
	struct binding *b, *best;
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
	
	kprintf("path = ");
	for (pp = path; pp; pp = pp->next)
		kprintf("/%s", pp->s);
	kprintf("\n");

	bestmatches = 0;
	best = nil;
	for (b = current->ngroup->bindings; b != nil; b = b->next) {
		matches = pathmatches(b->path, path);
		if (matches >= bestmatches) {
			bestmatches = matches;
			best = b;
		}
	}
	
	kprintf("found mount binding at: ");
	for (pp = best->path; pp; pp = pp->next)
		kprintf("/%s", pp->s);
	kprintf("\n");

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
	
	kprintf("so path from mount is: ");
	for (pp = mpath; pp; pp = pp->next)
		kprintf("/%s", pp->s);
	kprintf("\n");



	return ERR;
	/*
	return addpipe(current->fgroup, pipe);
	*/
}
