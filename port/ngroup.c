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
	n->lock = 0;
	n->bindings = nil;
	
	return n;
}

void
freengroup(struct ngroup *n)
{
	struct binding_list *b, *bb;
	
	lock(&n->lock);
	
	n->refs--;
	if (n->refs > 0) {
		unlock(&n->lock);
		return;
	}
		
	b = n->bindings;
	while (b != nil) {
		bb = b;
		b = b->next;
		freebinding(bb->binding);
		kfree(bb);
	}

	kfree(n);
}

struct ngroup *
copyngroup(struct ngroup *o)
{
	struct ngroup *n;
	struct binding_list *bo, *bn;
	
	lock(&o->lock);
	
	n = kmalloc(sizeof(struct ngroup));
	if (n == nil) {
		unlock(&o->lock);
		return nil;
	}
	
	bo = o->bindings;
	if (bo != nil) {
		n->bindings = kmalloc(sizeof(struct binding_list));
		bn = n->bindings;
	} else {
		n->bindings = nil;
	}
	
	while (bo != nil) {
		lock(&bo->binding->lock);
		
		bn->binding = bo->binding;
		bn->binding->refs++;
		
		unlock(&bo->binding->lock);
			
		bo = bo->next;
		if (bo == nil) {
			bn->next = nil;
		} else {
			bn->next = kmalloc(sizeof(struct binding_list));
		}
		
		bn = bn->next;
		
	}
	
	n->refs = 1;
	n->lock = 0;
	unlock(&o->lock);
	return n;
}
