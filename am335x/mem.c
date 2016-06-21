#include "dat.h"
#include "mem.h"
#include "../port/com.h"

extern void *_kernel_bin_start;
extern void *_kernel_bin_end;
extern void *_kernel_heap_start;
extern void *_kernel_heap_end;

static uint8_t *next;

static uint32_t l1[4096] __attribute__((__aligned__(16*1024)));

static void mmu_empty1(void);

static void
mmu_init(void)
{
	mmu_empty1();
	
	asm(
		/* Set ttb */
		"ldr r1, =l1			\n"
		"mcr p15, 0, r1, c2, c0, 0	\n"
		/* Disable domains ? */
		"mov r1, #3			\n"
		"mcr p15, 0, r1, c3, c0, 0	\n"
	);
}


void
memory_init(void)
{
	kprintf("bin_start = 0x%h\n", (uint32_t) &_kernel_bin_start);
	kprintf("bin_end   = 0x%h\n", (uint32_t) &_kernel_bin_end);

	kprintf("heap_start = 0x%h\n", (uint32_t) &_kernel_heap_start);
	kprintf("heap_end   = 0x%h\n", (uint32_t) &_kernel_heap_end);
	
	next = (uint8_t *) &_kernel_heap_start;

	mmu_init();
	
	/* Map kernel memory */	
	mmu_imap_section((uint32_t) &_kernel_bin_start, 
		(uint32_t) &_kernel_bin_end);

	/* Direct io map, for now. */
	mmu_imap_section(0x40000000, 0x4A400000);

//	mmu_enable();
}

void
mmu_invalidate(void)
{
	asm("mcr p15, 0, r1, c8, c7, 0");
}

void
mmu_switch(struct proc *p)
{
	struct page *page;
	
	mmu_empty1();
	
	for (page = p->page; page != nil; page = page->next) {
		l1[(int) page->va] = ((uint32_t) page->l2) | L1_COARSE;
	}

	mmu_invalidate();
}

void
mmu_enable(void)
{
	kprintf("mmu_enable\n");
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

void
mmu_imap_section(uint32_t start, uint32_t end)
{
	int id;
	
	start = SECTION_ALIGN(start);
	end = SECTION_ALIGN(end);
	
	kprintf("mmu_imap_section from 0x%h through 0x%h\n", 
		start, end);
	
	while (start <= end) {
		id = start >> SECTION_SHIFT;

		/* Map section so everybody can see it.
		 * This wil change. */
		l1[id] = ((uint32_t) start) | (3 << 10) | L1_SECTION;

		start += SECTION_SIZE;
	}
	
	mmu_invalidate();	
}

void *
kmalloc(size_t size)
{
	void *place = next;

	if (next + size >= (uint8_t*) &_kernel_heap_end) {
		kprintf("Kernel has run out of memory!\n");
		return (void *) nil;
	}
	
	next += size;
	
	return place;
}

void
kfree(void *ptr)
{

}

void
mmu_empty1(void)
{
	int i;
	for (i = 0; i < 4096; i++)
		l1[i] = L1_FAULT;
}