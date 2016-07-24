#include "dat.h"

reg_t
sysexit(va_list args)
{
	int i;
	
	int code = va_arg(args, int);
	
	kprintf("pid %i exited with status %i\n", 
		current->pid, code);

	for (i = 0; i < Smax; i++) {
		freeseg(current->segs[i]);
	}

	freepath(current->dot);	
	freefgroup(current->fgroup);
	freengroup(current->ngroup);
	
	procremove(current);
	schedule();
	
	/* Never reached. */
	return ERR;
}

reg_t
syssleep(va_list args)
{
	int ms;
	ms = va_arg(args, int);

	if (ms <= 0) {
		schedule();
		return 0;
	}

	procsleep(current, ms);
		
	return 0;
}

reg_t
sysfork(va_list args)
{
	int i;
	struct proc *p;
	int flags;
	bool copy;
	
	flags = va_arg(args, int);

	p = newproc();
	if (p == nil)
		return ERR;

	p->parent = current;
	p->quanta = current->quanta;
	p->dot = copypath(current->dot);

	copy = !(flags & FORK_smem);
	for (i = 0; i < Smax; i++) {
		p->segs[i] = copyseg(current->segs[i], copy || (i == Sstack));
		if (p->segs[i] == nil) {
			/* Should do better cleaning up here. */
			return ENOMEM;
		}
	}
	
	if (flags & FORK_sfgroup) {
		p->fgroup = current->fgroup;
		p->fgroup->refs++;
	} else {
		p->fgroup = copyfgroup(current->fgroup);
	}
	
	if (flags & FORK_sngroup) {
		p->ngroup = current->ngroup;
		p->ngroup->refs++;
	} else {
		p->ngroup = copyngroup(current->ngroup);
	}
	
	forkchild(p, current->ureg);
	procready(p);

	return p->pid;
}

reg_t
sysgetpid(va_list args)
{
	return current->pid;
}

static void
fixpages(struct page *pg, reg_t offset, struct page *next)
{
	struct page *p, *pp;

	pp = nil;
	for (p = pg; p != nil; pp = p, p = p->next)
		p->va = (uint8_t *) p->va + offset;
	
	pp->next = next;
}

reg_t
sysgetmem(va_list args)
{
	struct page *pages, *p, *pt;
	void *addr;
	size_t *size, s;
	
	size = va_arg(args, size_t *);

	kprintf("Get page %i\n", *size);
		
	pages = newpage(0);
	if (pages == nil) {
		return nil;
	}
	
	s = pages->size;
	p = pages;
	while (s < *size) {
		kprintf("get another page\n");
		p->next = newpage(p->va + p->size);
		if (p->next == nil) {
			p = pages;
			while (p != nil) {
				pt = p->next;
				freepage(p);
				p = pt;
			}
			return nil;
		}
		
		p = p->next;
		s += p->size;
	}
	
	*size = s;
	addr = nil;
	
	kprintf("Find a spot that will fit it\n");
	
	for (p = current->segs[Sheap]->pages; 
		p != nil && p->next != nil; 
		p = p->next) {
		
		addr = (uint8_t*) p->va + p->size;
		if ((size_t) (p->next->va - p->va) - p->size > s) {
			kprintf("found spot at 0x%h\n", addr);
			fixpages(pages, (reg_t) addr, p->next);
			p->next = pages;
			return (reg_t) addr;
		}
	}
	
	if (p != nil) {
		kprintf("append to segment\n");
		/* Append on end */
		addr = (uint8_t *) p->va + p->size;
		fixpages(pages, (reg_t) addr, nil);
		p->next = pages;
	} else {
		kprintf("set segment\n");
		
		for (p = current->segs[Stext]->pages;
			p != nil;
			p = p->next)
			if ((uint8_t *) p->va + p->size > (uint8_t *) addr)
				addr = (uint8_t *) p->va + p->size;

		for (p = current->segs[Sdata]->pages;
			p != nil;
			p = p->next)
			if ((uint8_t *) p->va + p->size > (uint8_t *) addr)
				addr = (uint8_t *) p->va + p->size;

		kprintf("can start putting pages at 0x%h\n", addr);	
		fixpages(pages, (reg_t) addr, nil);
		current->segs[Sheap]->pages = pages;
	}

	return (reg_t) addr;
}

reg_t
sysrmmem(va_list args)
{
	struct page *p, *pt, *pp;
	void *addr;
	size_t size;

	addr = va_arg(args, void *);
	size = va_arg(args, size_t);
	
	kprintf("Should unmap 0x%h of len %i from %i\n", addr, size, current->pid);
	
	pp = nil;
	p = current->segs[Sheap]->pages; 
	while (p != nil && size > 0) {
		if (p->va == addr) {
			kprintf("unmap page 0x%h\n", p->va);
			addr = (uint8_t *) p->va + p->size;
			size -= p->size;
			
			if (pp != nil) {
				pp->next = p->next;
			} else {
				current->segs[Sheap]->pages = p->next;
			}
			
			pt = p->next;
			freepage(p);
			p = pt;
		} else {
			pp = p;
			p = p->next;
		}
	}
	
	return ENOIMPL;
}
