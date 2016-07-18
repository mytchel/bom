#include "dat.h"

struct pipe {
	struct chan *c0, *c1;
	
};

bool
newpipe(struct chan **c0, struct chan **c1)
{
	*c0 = newchan(CHAN_pipe, O_RDWR, nil);
	if (*c0 == nil) {
		return false;
	}
	
	*c1 = newchan(CHAN_pipe, O_RDWR, nil);
	if (*c1 == nil) {
		freechan(*c0);
		return false;
	}
	
	(*c0)->aux = *c1;
	(*c1)->aux = *c0;
	
	return true;	
}

static int
piperead(struct chan *c, void *buf, int n)
{
	kprintf("should pipe read\n");
	return ERR;
}

static int
pipewrite(struct chan *c, void *buf, int n)
{
	kprintf("should pipe write\n");
	return ERR;
}

static int
pipeclose(struct chan *c)
{

	return ERR;
}

struct chantype devpipe = {
	&piperead,
	&pipewrite,
	&pipeclose,
};
