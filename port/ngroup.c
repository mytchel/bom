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
	n->bindings = nil;
	
	return n;
}

void
freengroup(struct ngroup *n)
{
	struct binding *b, *bb;
	
	n->refs--;
	if (n->refs > 0)
		return;
		
	b = n->bindings;
	while (b != nil) {
		bb = b;
		b = b->next;
		freepath(bb->path);
		freepipe(bb->pipe);
		kfree(bb);
	}

	kfree(n);
}

struct ngroup *
copyngroup(struct ngroup *o)
{
	struct ngroup *n;
	struct binding *bo, *bn;
	
	n = kmalloc(sizeof(struct ngroup));
	if (n == nil) {
		return nil;
	}
	
	bo = o->bindings;
	if (bo != nil) {
		n->bindings = kmalloc(sizeof(struct binding));
		bn = n->bindings;
	} else {
		n->bindings = nil;
	}
	
	while (bo != nil) {
		bn->path = bo->path;
		bn->path->refs++;
		bn->pipe = bo->pipe;
		bn->pipe->refs++;
		
		bo = bo->next;
		if (bo == nil) {
			bn->next = nil;
		} else {
			bn->next = kmalloc(sizeof(struct binding));
		}
		
		bn = bn->next;
		
	}
	
	n->refs = 1;
	return n;
}
