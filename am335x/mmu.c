#include "dat.h"
#include "mem.h"
#include "../port/com.h"

#define L1X(va)		(va >> 20)
#define L2X(va)		((va >> 12) & ((1 << 8) - 1))

uint32_t ttb[4096] __attribute__((__aligned__(16*1024)));

void
mmuswitch(struct proc *p)
{
	struct page *pg;

	kprintf("mmuswitch\n");	
	mmuempty1();

	for (pg = p->mmu; pg != nil; pg = pg->next) {
		ttb[L1X((uint32_t) pg->va)] 
			= ((uint32_t) pg->pa) | L1_COARSE;
	}

	kprintf("mmuswitch done\n");
}

void
imap(uint32_t start, uint32_t end)
{
	start &= ~((1 << 20) - 1);
	
	while (start < end) {
		/* Map section so everybody can see it.
		 * This wil change. */
		ttb[L1X(start)] = ((uint32_t) start) | (3 << 10) | L1_SECTION;
		start += 1 << 20;
	}
	
	mmuinvalidate();	
}

void
mmuempty1(void)
{
	int i;
	for (i = 0; i < 4096; i++) {
		if ((ttb[i] & L1_COARSE) == L1_COARSE)
			ttb[i] = L1_FAULT;
	}
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
	
	kprintf("ttb[%i] = 0x%h\n", L1X(x), ttb[L1X(x)]);
	
	l1 = &ttb[L1X(x)];
	
	/* Add a l1 page if needed. */
	if (*l1 == L1_FAULT) {
		kprintf("need to add l2 page table at index %i\n", L1X(x));
		
		pg = newpage((void *) (x & ~((1 << 20) - 1)));

		kprintf("new page at \npa: 0x%h\nva: 0x%h\n", 
			(uint32_t) pg->pa, (uint32_t) pg->va);
		
		kprintf("clear\n");
		for (i = 0; i < 256; i++)
			((uint32_t *) pg->pa)[i] = L2_FAULT;

		kprintf("add to current mmu list\n");	
		pg->next = current->mmu;
		current->mmu = pg;
		
		kprintf("set l1 page table to point to new table.\n");
		*l1 = ((uint32_t) pg->pa) | L1_COARSE;
	}
	
	kprintf("l1 page entry = 0x%h\n", *l1);
	kprintf("so l1 page a  = 0x%h\n", (*l1 & ~((1 << 10) - 1)));
	
	l2 = (uint32_t *) (*l1 & ~((1 << 10) - 1));

	kprintf("l2 = 0x%h\n", (uint32_t) l2);

	kprintf("put page into table\n");	
	l2[L2X(x)] = ((uint32_t) p->pa) | 0xff0 | L2_SMALL;
	kprintf("Should be good\n");
	
	kprintf("0x%h should translate to 0x%h\n", p->va, p->pa);
	kprintf("l2[%i] = 0x%h\n", L2X(x), l2[L2X(x)]);
	
	mmuinvalidate();
}
