#include "dat.h"
#include "fns.h"

#define L1X(va)		(va >> 20)
#define L2X(va)		((va >> 12) & ((1 << 8) - 1))

uint32_t ttb[4096] __attribute__((__aligned__(16*1024)));

void
initmmu(void)
{
	int i;
	for (i = 0; i < 4096; i++)
		ttb[i] = L1_FAULT;

	mmuloadttb(ttb);
}

void
mmuswitch(struct proc *p)
{
	struct page *pg;

	mmuempty1();

	for (pg = p->mmu; pg != nil; pg = pg->next) {
		ttb[L1X((uint32_t) pg->va)] 
			= ((uint32_t) pg->pa) | L1_COARSE;
	}
	
	mmuinvalidate();
}

void
imap(void *start, void *end, int ap)
{
	uint32_t x = (uint32_t) start & ~((1 << 20) - 1);
	
	while (x < (uint32_t) end) {
		/* Map section so everybody can see it.
		 * This wil change. */
		ttb[L1X(x)] = x | (ap << 10) | L1_SECTION;
		x += 1 << 20;
	}
}

void
mmuempty1(void)
{
	uint32_t i;

	for (i = 0; i < 4096; i++) {
		if ((ttb[i] & L1_COARSE) == L1_COARSE)
			ttb[i] = L1_FAULT;
	}
}

void
mmuputpage(struct page *p, bool rw)
{
	struct page *pg;
	uint32_t i, ap;
	uint32_t *l1, *l2;

	uint32_t x = (uint32_t) p->va;
	
	l1 = &ttb[L1X(x)];
	
	/* Add a l1 page if needed. */
	if (*l1  == L1_FAULT) {
		pg = newpage((void *) (x & ~((1 << 20) - 1)));

		for (i = 0; i < 256; i++)
			((uint32_t *) pg->pa)[i] = L2_FAULT;

		pg->next = current->mmu;
		current->mmu = pg;
		
		*l1 = ((uint32_t) pg->pa) | L1_COARSE;
	}
	
	l2 = (uint32_t *) (*l1 & ~((1 << 10) - 1));

	if (rw)
		ap = AP_RW_RW;
	else
		ap = AP_RW_RO;

	l2[L2X(x)] = ((uint32_t) p->pa) | (ap << 4) | L2_SMALL;
	
	mmuinvalidate();
}
