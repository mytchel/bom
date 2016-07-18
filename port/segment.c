#include "dat.h"

struct segment *
newseg(int type, void *base, size_t size)
{
	struct segment *s;
	
	s = kmalloc(sizeof(struct segment));
	if (s == nil) {
		return nil;
	}

	s->refs = 1;	
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
	
	s->refs--;
	if (s->refs > 0)
		return;
	
	p = s->pages;
	while (p != nil) {
		pp = p;
		p = p->next;
		freepage(pp);
	}
	
	kfree(s);
}

struct segment *
copyseg(struct segment *s, bool copy)
{
	struct segment *n;
	struct page *sp, *np;

	if (s->type == SEG_ro || !copy) {
		/* No need to actually copy. */
		s->refs++;
		n = s;
	} else {
		/* Gets new pages and copies the data from old. */
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
	}
	
	return n;
}

static struct segment *
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

static struct page *
findpage(struct segment *s, void *addr)
{
	struct page *pg;

	for (pg = s->pages; pg != nil; pg = pg->next) {
		if (pg->va <= addr && pg->va + pg->size > addr) {
			return pg;
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
	if (s == nil) return false;
	
	pg = findpage(s, addr);
	if (pg == nil) return false;
	
	mmuputpage(pg, s->type == SEG_rw);
	current->faults++;
	
	return true;
}

void *
kaddr(struct proc *p, void *addr)
{
	struct segment *s;
	struct page *pg;
	
	s = findseg(current, addr);
	if (s == nil) return nil;
	
	pg = findpage(s, addr);
	if (pg == nil) return nil;

	return pg->pa + (addr - pg->va);
}
