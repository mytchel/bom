#include "dat.h"

struct segment *
newseg(int type, void *base, size_t size)
{
	struct segment *s;
	
	s = kmalloc(sizeof(struct segment));
	if (s == nil) {
		return nil;
	}
	
	s->type = type;
	s->base = base;
	s->top = (void *) ((uint32_t) base + size);
	s->size = size;
	
	return s;
}

void
freeseg(struct segment *s)
{
	struct page *p, *pp;
	
	p = s->pages;
	while (p != nil) {
		pp = p;
		p = p->next;
		untagpage(pp->pa);
		kfree(pp);
	}
	
	kfree(s);
}

struct segment *
findseg(struct proc *p, void *addr)
{
	struct segment *s;
	int i;
	
	for (i = 0; i < Smax; i++) {
		s = p->segs[i];
		if (s == nil)
			continue;
		
		if (s->base <= addr && s->top > addr) {
			return s;
		}
	}
	
	return nil;
}

struct segment *
copyseg(struct segment *s, bool new)
{
	struct segment *n;
	struct page *sp, *np;
	
	n = newseg(s->type, s->base, s->size);
	if (n == nil)
		return nil;

	sp = s->pages;
	n->pages = newpage(sp->va);
	np = n->pages;
	while (sp != nil) {
		memmove(np->pa, sp->pa, sp->size);
		if (sp->next == nil)
			break;
		
		sp = sp->next;
		np->next = newpage(sp->va);
		np = np->next;
	}
	
	return n;
}

bool
fixfault(void *addr)
{
	struct segment *s;
	struct page *pg;
	
	s = findseg(current, addr);
	if (s == nil)
		return false;
	
	for (pg = s->pages; pg != nil; pg = pg->next) {
		if (pg->va <= addr && pg->va + pg->size > addr) {
			mmuputpage(pg);
			current->faults++;
			return true;
		}
	}
	
	return false;
}
