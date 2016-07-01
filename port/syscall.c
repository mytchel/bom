#include "dat.h"

static int
sysexit(va_list args)
{
	int i;
	struct segment *s;
	
	int code = va_arg(args, int);
	
	kprintf("pid %i exited with status %i\n", 
		current->pid, code);

	for (i = 0; i < Smax; i++) {
		s = current->segs[i];
		if (s != nil)
			freeseg(s);
	}

	procremove(current);
	schedule();
	
	/* Never reached. */
	return 1;
}

static int
syssleep(va_list args)
{
	int ms;
	ms = va_arg(args, int);

	if (ms <= 0) {
		schedule();
		return 0;
	}

	procsleep(current, ms);
		
	return 0;
}

static int
sysfork(va_list args)
{
	int i;
	struct proc *p;
	/*
	int flags;
	
	flags = va_arg(args, int);
	*/
	p = newproc();
	if (p == nil)
		return -1;

	for (i = 0; i < Smax; i++) {
		if (current->segs[i] != nil) {
			p->segs[i] = copyseg(current->segs[i], true);
			if (p->segs[i] == nil)
				return -1;
		}
	}

	forkchild(p, current->ureg);
	
	p->parent = current;
	p->quanta = current->quanta;
	
	p->fgroup = current->fgroup;
	p->fgroup->refs++;

	procready(p);
	return p->pid;
}

static int
sysgetpid(va_list args)
{
	return current->pid;
}

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
		
		f->files = files;
		f->nfiles *= 2;
	}
	
	f->files[fd] = pipe;
	return fd;
}

static int
syspipe(va_list args)
{
	int **fds, *kfds;
	struct pipe *p1, *p2;
	
	fds = va_arg(args, int**);
	kfds = (int *) kaddr(current, (void *) fds);

	p1 = kmalloc(sizeof(struct pipe));
	if (p1 == nil)
		return -1;

	p2 = kmalloc(sizeof(struct pipe));
	if (p2 == nil) {
		kfree(p1);
		return -1;
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

static int
sysclose(va_list args)
{
	int fd;
	fd = va_arg(args, int);
	
	kprintf("should close %i\n", fd);
	return -1;
}

static int
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
		return -1;
	pipe = current->fgroup->files[fd];
	if (pipe == nil)
		return -1;
	
	kbuf = kaddr(current, buf);

	pipe->user = current;	
	pipe->buf = kbuf;
	pipe->n = n;

	procwait(current);
	schedule();
	
	return pipe->n;
}

static int
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
		return -1;

	pipe = current->fgroup->files[fd];
	if (pipe == nil)
		return -1;
	
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

int (*syscalltable[NSYSCALLS])(va_list) = {
	[SYSCALL_EXIT] 		= sysexit,
	[SYSCALL_FORK] 		= sysfork,
	[SYSCALL_SLEEP]		= syssleep,
	[SYSCALL_GETPID]	= sysgetpid,
	
	[SYSCALL_PIPE]		= syspipe,
	[SYSCALL_CLOSE]		= sysclose,
	[SYSCALL_READ]		= sysread,
	[SYSCALL_WRITE]		= syswrite,
};
