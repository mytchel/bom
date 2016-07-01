#include "dat.h"

static int
addfd(struct fgroup *f, struct pipe *pipe)
{
	int i, fd;
	
	for (i = 0; i < f->nfiles; i++)
		if (f->files[i] == nil)
			break;
	
	if (f->files[i] == nil) {
		fd = i;
	} else {
		/* Need to grow file table. */
		struct pipe **files;
		
		files = kmalloc(sizeof(struct file **) * f->nfiles * 2);
		if (files == nil)
			return -1;
		
		for (i = 0; i < f->nfiles; i++)
			files[i] = f->files[i];

		fd = i++;

		while (i < f->nfiles * 2)
			files[i++] = nil;

		kfree(f->files);
				
		f->files = files;
		f->nfiles *= 2;
	}
	
	f->files[fd] = pipe;
	return fd;
}

int
syspipe(va_list args)
{
	int **fds, *kfds;
	struct pipe *p1, *p2;
	
	fds = va_arg(args, int**);
	kfds = (int *) kaddr(current, (void *) fds);

	p1 = kmalloc(sizeof(struct pipe));
	if (p1 == nil)
		return SYS_ERR;

	p2 = kmalloc(sizeof(struct pipe));
	if (p2 == nil) {
		kfree(p1);
		return SYS_ERR;
	}
	
	p1->refs = p2->refs = 1;
	p1->user = p2->user = nil;
	p1->buf = p2->buf = nil;
	p1->n = p2->n = 0;
	
	p1->link = p2;
	p2->link = p1;
	
	kfds[0] = addfd(current->fgroup, p1);
	kfds[1] = addfd(current->fgroup, p2);
	
	return 0;
}

int
sysclose(va_list args)
{
	int fd;
	struct pipe *pipe;
	
	fd = va_arg(args, int);
	
	kprintf("should close %i\n", fd);
	
	if (fd >= current->fgroup->nfiles)
		return SYS_ERR;
	
	pipe = current->fgroup->files[fd];
	if (pipe == nil)
		return SYS_ERR;
	
	/* Remove fd. */
	current->fgroup->files[fd] = nil;
	
	pipe->refs--;
	if (pipe->refs == 0) {
		if (pipe->link) {
			pipe->link->link = nil;
		}
		
		kfree(pipe);
	}
	
	return 0;
}

int
sysread(va_list args)
{
	int fd;
	size_t n;
	void *buf, *kbuf;
	struct pipe *pipe;

	fd = va_arg(args, int);
	buf = va_arg(args, void *);
	n = va_arg(args, size_t);
	
	if (fd >= current->fgroup->nfiles)
		return SYS_ERR;

	pipe = current->fgroup->files[fd];
	if (pipe == nil)
		return SYS_ERR;

	if (pipe->link == nil)
		return SYS_ERRPC;
	
	kbuf = kaddr(current, buf);

	pipe->user = current;	
	pipe->buf = kbuf;
	pipe->n = n;

	procwait(current);
	schedule();
	
	return pipe->n;
}

int
syswrite(va_list args)
{
	int fd;
	size_t n, i;
	void *buf, *kbuf;
	char *in, *out;
	struct pipe *pipe;

	fd = va_arg(args, int);
	buf = va_arg(args, void *);
	n = va_arg(args, size_t);
	
	if (fd >= current->fgroup->nfiles)
		return SYS_ERR;

	pipe = current->fgroup->files[fd];
	if (pipe == nil)
		return SYS_ERR;
	
	if (pipe->link == nil)
		return SYS_ERRPC;
	
	kbuf = kaddr(current, buf);

	pipe->user = current;	
	pipe->buf = kbuf;
	pipe->n = n;

	while (pipe->link->user == nil)
		schedule();

	in = (char *) pipe->buf;
	out = (char *) pipe->link->buf;	
	for (i = 0; i < pipe->n && i < pipe->link->n; i++) {
		out[i] = in[i];
	}

	pipe->link->n = i;

	procready(pipe->link->user);

	pipe->user = nil;
	pipe->link->user = nil;

	return i;
}
