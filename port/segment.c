#include "dat.h"

struct segment *
newseg(int type)
{
	struct segment *s;
	
	s = malloc(sizeof(struct segment));
	if (s == nil) {
		return nil;
	}

	s->refs = 1;	
	s->type = type;
	s->pages = nil;
	
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
	
	free(s);
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
		n = newseg(s->type);
		if (n == nil)
			return nil;

		sp = s->pages;
		if (sp == nil)
			return n;

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

static struct page *
findpage(struct proc *p, void *addr, struct segment **seg)
{
	struct segment *s;
	struct page *pg;
	int i;

	for (i = 0; i < Smax; i++) {
		s = p->segs[i];
		for (pg = s->pages; pg != nil; pg = pg->next) {
			if (pg->va <= addr && pg->va + pg->size > addr) {
				*seg = s;
				return pg;
			}
		}
	}
	
	return nil;
}

bool
fixfault(void *addr)
{
	struct segment *s;
	struct page *pg;
	
	pg = findpage(current, addr, &s);
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
	
	pg = findpage(p, addr, &s);
	if (pg == nil) return nil;

	return pg->pa + (addr - pg->va);
}
