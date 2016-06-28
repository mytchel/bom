#include "dat.h"
#include "mem.h"
#include "../port/com.h"

#define L1X(va)		(va >> 20)
#define L2X(va)		((va >> 12) & ((1 << 8) - 1))

static uint32_t mmul1[4096] __attribute__((__aligned__(16*1024)));

int
mmuinit(void)
{
	mmuempty1();
	
	asm(
		/* Set ttb */
		"ldr r1, =mmul1			\n"
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
	struct page *pg;
	
	mmuempty1();

	for (pg = p->mmu; pg != nil; pg = pg->next) {
		mmul1[L1X((uint32_t) pg->va)] 
			= ((uint32_t) pg->pa) | L1_COARSE;
	}

	mmuinvalidate();
}

void
imap(uint32_t start, uint32_t end)
{
	start &= ~((1 << 20) - 1);
	
	while (start < end) {
		/* Map section so everybody can see it.
		 * This wil change. */
		mmul1[L1X(start)] = ((uint32_t) start) | (3 << 10) | L1_SECTION;
		start += 1 << 20;
	}
	
	mmuinvalidate();	
}

void
mmuempty1(void)
{
	int i;
	for (i = 0; i < 4096; i++)
		mmul1[i] = L1_FAULT;
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

void
mmuputpage(struct page *p)
{
	struct page *pg;
	int i;
	uint32_t *l1, *l2;

	kprintf("mmuputpage\n");
		
	uint32_t x = (uint32_t) p->va;

	kprintf("page va 0x%h\n", x);
	
	l1 = &mmul1[L1X(x)];

	/* Add a l1 page if needed. */
	if (*l1 == L1_FAULT) {
		kprintf("need to add l2 page table\n");
		
		pg = newpage((void *) (x & ~((1 << 20) - 1)));

		kprintf("new page at \npa: 0x%h\nva: 0x%h\n", 
			(uint32_t) pg->pa, (uint32_t) pg->va);
		
		l1[L1X(x)] = ((uint32_t) pg->pa) | L1_COARSE;

		kprintf("clear\n");		
		for (i = 0; i < 256; i++)
			((uint32_t *) pg->pa)[i] = L2_FAULT;

		kprintf("add to current mmu list\n");	
		pg->next = current->mmu;
		current->mmu = pg;
		
		kprintf("set l1 page table to point to new table.\n");
		*l1 = (uint32_t) pg->pa;
	}
	
	kprintf("l2 page table addr = 0x%h\n", (uint32_t) *l1);
	
	l2 = (uint32_t *) *l1;

	kprintf("put page into table\n");	
	l2[L2X(x)] = ((uint32_t) p->pa) | 0xff0 | L2_SMALL;
	kprintf("Should be good\n");
	
	mmuinvalidate();
}
