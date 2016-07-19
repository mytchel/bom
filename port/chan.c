#include "dat.h"

struct chantype *chantypes[CHAN_max] = {
	[CHAN_pipe] = &devpipe,
	[CHAN_file] = &devfile,
};

struct chan *
newchan(int type, int flags, struct path *p)
{
	struct chan *c;
	
	c = kmalloc(sizeof(struct chan));
	if (c == nil) {
		return nil;
	}

	c->refs = 1;
	c->lock = 0;
	c->type = type;
	c->flags = flags;
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

	kprintf("Actually free chan\n");	
	chantypes[c->type]->close(c);
	freepath(c->path);
	kfree(c);
}
