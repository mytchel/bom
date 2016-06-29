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
	s->top = base + size * PAGE_SIZE;
	
	return s;
}

struct segment *
findseg(struct proc *p, void *addr)
{
	struct segment *s;
	int i;
	
	for (i = 0; i < NSEG; i++) {
		s = p->segs[i];
		if (s == nil)
			continue;
		
		if (s->base <= addr && s->top > addr) {
			return s;
		}
	}
	
	return nil;
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
		if (pg->va <= addr && pg->va + pg->size  > addr) {
			mmuputpage(pg);
			return true;
		}
	}
	
	return false;
}
