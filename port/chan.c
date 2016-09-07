#include "head.h"

struct chantype *chantypes[CHAN_max] = {
	[CHAN_pipe] = &devpipe,
	[CHAN_file] = &devfile,
};

struct chan *
newchan(int type, int mode, struct path *p)
{
	struct chan *c;
	
	c = malloc(sizeof(struct chan));
	if (c == nil) {
		return nil;
	}

	c->refs = 1;
	c->lock = 0;
	c->type = type;
	c->mode = mode;
	c->path = p;
	
	return c;
}

void
freechan(struct chan *c)
{
	lock(&c->lock);
	
	c->refs--;
	if (c->refs > 0) {
		unlock(&c->lock);
		return;
	}

	chantypes[c->type]->close(c);
	freepath(c->path);
	free(c);
}
