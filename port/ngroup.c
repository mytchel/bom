#include "dat.h"

struct ngroup *
newngroup(void)
{
	struct ngroup *n;
	
	n = kmalloc(sizeof(struct ngroup));
	if (n == nil) {
		return nil;
	}

	n->refs = 1;
	
	return n;
}

void
freengroup(struct ngroup *n)
{
	n->refs--;
	if (n->refs > 0)
		return;

	kfree(n);
}

struct ngroup *
copyngroup(struct ngroup *o)
{
	struct ngroup *n;
	
	n = kmalloc(sizeof(struct ngroup));
	if (n == nil) {
		return nil;
	}
	
	n->refs = 1;
	return n;
}
