#include "dat.h"

struct binding *
newbinding(struct path *path, struct chan *out, struct chan *in)
{
	struct binding *b;
	
	b = malloc(sizeof(struct binding));
	if (b == nil)
		return nil;
	
	b->refs = 1;
	b->lock = 0;

	b->path = path;
	b->in = in;
	b->out = out;
	b->rootfid = ROOTFID;
	
	in->refs++;
	out->refs++;

	b->resp = nil;
	b->waiting = nil;
	b->nreqid = 0;
	
	return b;
}

void
freebinding(struct binding *b)
{
	lock(&b->lock);
	
	b->refs--;
	
	unlock(&b->lock);
	
	/* 
	 * Doesnt need to do any freeing
	 * when mountproc sees that refs <=0
	 * it will do freeing.
	 */
}
