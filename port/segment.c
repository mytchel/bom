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
copyseg(struct segment *s, bool new)
{
	struct segment *n;
	struct page *sp, *np;
	
	n = newseg(s->type, s->base, s->size);
	if (n == nil)
		return nil;

	sp = s->pages;

	switch (s->type) {
	case SEG_RO:
		/* Doesn't make new pages, just references the old ones 
		 * from new page structs. */
		 
		n->pages = kmalloc(sizeof(struct page));
		np = n->pages;
		while (sp != nil) {
			kprintf("make new page point to old 0x%h -> 0x%h\n", sp->va, sp->pa);
			np->pa = sp->pa;
			np->va = sp->va;
			np->size = sp->size;
			
			tagpage(np->pa);
			
			sp = sp->next;
			if (sp)
				np->next = kmalloc(sizeof(struct page));
			else
				np->next = nil;	
			np = np->next;
		}
	
		break;
	case SEG_RW:
		/* Gets new pages and copies the data from old. */

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

		break;
	}
	
	return n;
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
