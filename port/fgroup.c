#include "dat.h"

struct fgroup *
newfgroup(void)
{
	struct fgroup *f;
	
	f = kmalloc(sizeof(struct fgroup));
	if (f == nil) {
		return nil;
	}

	f->pipes = kmalloc(sizeof(struct pipe*));
	if (f->pipes == nil) {
		kfree(f);
		return nil;
	}
	
	f->pipes[0] = nil;
	f->npipes = 1;
	f->refs = 1;	
	
	return f;
}

void
freefgroup(struct fgroup *f)
{
	int i;
	struct pipe *p;
	
	f->refs--;
	if (f->refs > 0)
		return;

	for (i = 0; i < f->npipes; i++) {
		p = f->pipes[i];
		if (p != nil)
			freepipe(p);
	}
	
	kfree(f->pipes);
	kfree(f);
}

struct fgroup *
copyfgroup(struct fgroup *fo)
{
	int i;
	struct fgroup *f;
	
	f = kmalloc(sizeof(struct fgroup));
	if (f == nil) {
		return nil;
	}

	f->pipes = kmalloc(sizeof(struct pipe*) * fo->npipes);
	if (f->pipes == nil) {
		kfree(f);
		return nil;
	}
	
	f->npipes = fo->npipes;
	for (i = 0; i < f->npipes; i++) {
		f->pipes[i] = fo->pipes[i];
		if (f->pipes[i] != nil)
			f->pipes[i]->refs++;
	}
	
	f->refs = 1;
	
	return f;
}

int
addpipe(struct fgroup *f, struct pipe *pipe)
{
	int i, fd;
	
	for (i = 0; i < f->npipes; i++)
		if (f->pipes[i] == nil)
			break;
	
	if (f->pipes[i] == nil) {
		fd = i;
	} else {
		/* Need to grow file table. */
		struct pipe **pipes;
		
		pipes = kmalloc(sizeof(struct file **) * f->npipes * 2);
		if (pipes == nil)
			return -1;
		
		for (i = 0; i < f->npipes; i++)
			pipes[i] = f->pipes[i];

		fd = i++;

		while (i < f->npipes * 2)
			pipes[i++] = nil;

		kfree(f->pipes);
				
		f->pipes = pipes;
		f->npipes *= 2;
	}
	
	pipe->refs++;
	f->pipes[fd] = pipe;
	return fd;
}

struct pipe *
fdtopipe(struct fgroup *f, int fd)
{
	if (fd < f->npipes)
		return f->pipes[fd];
	else
		return nil;
}
