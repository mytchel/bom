#include "dat.h"
#include "mem.h"
#include "../port/com.h"

extern void *_ram_start;
extern void *_ram_end;
extern void *_kernel_bin_start;
extern void *_kernel_bin_end;

static uint32_t l1lo, l1hi;
static uint32_t l1[4096] __attribute__((__aligned__(16*1024)));

#define PAGE_UNUSED	1
#define PAGE_USED	1

static uint32_t npages;
static uint32_t *pages;

void
mmu_init(void)
{
	uint32_t i, ram_size;
	
	l1lo = 0;
	l1hi = 0;

	ram_size = (uint32_t) &_ram_end - (uint32_t) &_ram_start;
	npages = ram_size / PAGE_SIZE;

	kprintf("ram start = 0x%h\n", &_ram_start);	
	kprintf("ram end   = 0x%h\n", &_ram_end);
	
	kprintf("ram = 0x%h (0x%h)\n", ram_size, npages);
	
	pages = kmalloc(sizeof(uint32_t) * npages);
	for (i = 0; i < npages; i++)
		pages[i] = PAGE_UNUSED;

	mmu_empty1();
	
	asm(
		/* Set ttb */
		"ldr r1, =l1			\n"
		"mcr p15, 0, r1, c2, c0, 0	\n"
		/* Set domain mode to manager (for now) */
		"mov r1, #3			\n"
		"mcr p15, 0, r1, c3, c0, 0	\n"
	);

	mmu_imap_section((uint32_t) &_kernel_bin_start,
		(uint32_t) &_kernel_bin_end);
		
	mmu_imap_section(0x40000000, 0x4A400000);
}

void
mmu_switch(struct proc *p)
{
	mmu_empty1();
/*	
	for (page = p->page; page != nil; page = page->next) {
		i = ((uint32_t) page->va) >> 20;
		l1[i] = ((uint32_t) page->l2) | L1_COARSE;
		
		if (i < l1lo) l1lo = i;
		else if (i > l1hi) l1hi = i;
	}
*/
	mmu_invalidate();
}

void
mmu_imap_section(uint32_t start, uint32_t end)
{
	start &= ~((1 << 20) - 1);
	
	while (start < end) {
		/* Map section so everybody can see it.
		 * This wil change. */
		l1[start >> 20] = ((uint32_t) start) | (3 << 10) | L1_SECTION;
		start += 1 << 20;
	}
	
	mmu_invalidate();	
}

void
mmu_empty1(void)
{
	int i;
	for (i = l1lo; i < l1hi; i++)
		l1[i] = L1_FAULT;
		
	l1lo = 0;
	l1hi = 0;
}

void
mmu_invalidate(void)
{
	asm("mcr p15, 0, r1, c8, c7, 0");
}

void
mmu_enable(void)
{
	mmu_invalidate();
	asm(
		/* Enable mmu */
		"mrc p15, 0, r1, c1, c0, 0	\n"
		"orr r1, r1, #1			\n"
		"mcr p15, 0, r1, c1, c0, 0	\n"
	);
}

void
mmu_disable(void)
{
	asm(
		"mrc p15, 0, r1, c1, c0, 0	\n"
		"bic r1, r1, #1			\n"
		"mcr p15, 0, r1, c1, c0, 0	\n"
	);
}
