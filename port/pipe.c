#include "dat.h"

struct pipe *
newpipe(void)
{
	struct pipe *p;
	
	p = kmalloc(sizeof(struct pipe));
	if (p == nil) {
		return nil;
	}

	p->refs = 1;
	
	p->action = PIPE_none;
	p->user = nil;
	
	return p;
}

void
freepipe(struct pipe *p)
{
	p->refs--;
	if (p->refs > 0)
		return;
	
	if (p->link != nil) {
		p->link->link = nil;
	}
	
	kfree(p);
}
