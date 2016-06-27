#include "dat.h"
#include "mem.h"

static uint32_t l1lo, l1hi;
static uint32_t l1[4096] __attribute__((__aligned__(16*1024)));

int
mmuinit(void)
{
	l1lo = 0;
	l1hi = 4096;

	mmuempty1();
	
	asm(
		/* Set ttb */
		"ldr r1, =l1			\n"
		"mcr p15, 0, r1, c2, c0, 0	\n"
		/* Set domain mode to manager (for now) */
		"mov r1, #3			\n"
		"mcr p15, 0, r1, c3, c0, 0	\n"
	);
	
	return 0;
}

void
mmuswitch(struct proc *p)
{
	mmuempty1();
/*	
	for (page = p->page; page != nil; page = page->next) {
		i = ((uint32_t) page->va) >> 20;
		l1[i] = ((uint32_t) page->l2) | L1_COARSE;
		
		if (i < l1lo) l1lo = i;
		else if (i > l1hi) l1hi = i;
	}
*/
	mmuinvalidate();
}

void
imap(uint32_t start, uint32_t end)
{
	start &= ~((1 << 20) - 1);
	
	while (start < end) {
		/* Map section so everybody can see it.
		 * This wil change. */
		l1[start >> 20] = ((uint32_t) start) | (3 << 10) | L1_SECTION;
		start += 1 << 20;
	}
	
	mmuinvalidate();	
}

void
mmuempty1(void)
{
	int i;
	for (i = l1lo; i < l1hi; i++)
		l1[i] = L1_FAULT;
		
	l1lo = 0;
	l1hi = 0;
}

void
mmuinvalidate(void)
{
	asm("mcr p15, 0, r1, c8, c7, 0");
}

void
mmuenable(void)
{
	mmuinvalidate();
	asm(
		/* Enable mmu */
		"mrc p15, 0, r1, c1, c0, 0	\n"
		"orr r1, r1, #1			\n"
		"mcr p15, 0, r1, c1, c0, 0	\n"
	);
}

void
mmudisable(void)
{
	asm(
		"mrc p15, 0, r1, c1, c0, 0	\n"
		"bic r1, r1, #1			\n"
		"mcr p15, 0, r1, c1, c0, 0	\n"
	);
}

