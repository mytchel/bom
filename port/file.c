#include "dat.h"

struct chan *
fileopen(struct binding *b, struct path *path, int flag, int *err)
{
	*err = ERR;
	kprintf("file open\n");
	
	char buf[] = "hi";
	
	pipewrite(b->out, buf, 3);
	
	return nil;
}

int
fileread(struct chan *c, char *buf, size_t n)
{
	return ERR;
}

int
filewrite(struct chan *c, char *buf, size_t n)
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
