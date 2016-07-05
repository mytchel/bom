#include "dat.h"

int
syspipe(va_list args)
{
	int *fds, *kfds;
	struct pipe *p1, *p2;
	
	fds = va_arg(args, int*);
	kfds = (int *) kaddr(current, (void *) fds);

	p1 = newpipe();
	if (p1 == nil) {
		return ERR;
	}
	
	p2 = newpipe();
	if (p2 == nil) {
		kfree(p1);
		return ERR;
	}
	
	p1->path = p2->path = nil;
	
	p1->link = p2;
	p2->link = p1;
	
	kfds[0] = addpipe(current->fgroup, p1);
	kfds[1] = addpipe(current->fgroup, p2);
	
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
	}
	
	kprintf("should read %i bytes from %i into 0x%h\n", 
		n, fd, buf);
	
	return ERR;
}

int
syswrite(va_list args)
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
	}

	kprintf("should write %i bytes from 0x%h into %i\n", 
		n, buf, fd);
	
	return ERR;
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
	const char *upath;
	
	fd = va_arg(args, int);
	upath = va_arg(args, const char *);
	flags = va_arg(args, int);
	
	kprintf("bind %i to '%s' with flags 0b%h\n", fd, upath, flags);

	return ERR;
}

int
sysopen(va_list args)
{
	int flags, mode;
	const char *upath;
	struct path *path;
	
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
	
	struct path *pp;
	kprintf("path = ");
	for (pp = path; pp; pp = pp->next)
		kprintf("/%s", pp->s);
	kprintf("\n");

	return ERR;
	/*
	return addpipe(current->fgroup, pipe);
	*/
}
