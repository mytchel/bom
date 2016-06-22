#include "dat.h"
#include "mem.h"
#include "../port/com.h"

extern void *_ram_start;
extern void *_ram_end;
extern void *_kernel_bin_start;
extern void *_kernel_bin_end;
extern void *_kernel_heap_start;
extern void *_kernel_heap_end;

static int l1lo, l1hi;
static uint32_t l1[4096] __attribute__((__aligned__(16*1024)));

static uint8_t *knext;

#define PAGE_UNUSED	1
#define PAGE_USED	1
static uint8_t npages, *pages;

void
memory_init(void)
{
	uint32_t i, ram_size;
	
	kprintf("ram_start = 0x%h\n", (uint32_t) &_ram_start);
	kprintf("ram_end   = 0x%h\n", (uint32_t) &_ram_end);

	kprintf("bin_start = 0x%h\n", (uint32_t) &_kernel_bin_start);
	kprintf("bin_end   = 0x%h\n", (uint32_t) &_kernel_bin_end);

	kprintf("heap_start = 0x%h\n", (uint32_t) &_kernel_heap_start);
	kprintf("heap_end   = 0x%h\n", (uint32_t) &_kernel_heap_end);
	
	knext = (uint8_t *) &_kernel_heap_start;
	
	ram_size = (uint32_t) &_ram_end - (uint32_t) &_ram_start;
	npages = ram_size / PAGE_SIZE;

	pages = kmalloc(sizeof(uint32_t) * npages);
	if (pages == nil)
		panic("Failed to kmalloc pages.\n");
		
	for (i = 0; i < npages; i++)
		pages[i] = PAGE_UNUSED;

	mmu_init();
	
	/* Map kernel memory */	
	mmu_imap_section((uint32_t) &_kernel_bin_start, 
		(uint32_t) &_kernel_bin_end);

	/* Direct io map, for now. */
	mmu_imap_section(0x40000000, 0x4A400000);

	mmu_enable();
}

void
mmu_switch(struct proc *p)
{
	kprintf("mmu_switch\n");
	
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
	
	kprintf("mmu switched\n");
}

void
mmu_imap_section(uint32_t start, uint32_t end)
{
	int i;	
	
	start &= ~((1 << 20) - 1);
	
	for (i = start; i < end; i += 1 << 12) {
		pages[i >> 12] = PAGE_USED;
	}
	
	for (i = start; i < end; i += 1 << 20) {
		/* Map section so everybody can see it.
		 * This wil change. */
		l1[i >> 20] = ((uint32_t) i) | (3 << 10) | L1_SECTION;
	}
	
	mmu_invalidate();	
}

void *
kmalloc(size_t size)
{
	void *place = knext;

	if (knext + size >= (uint8_t*) &_kernel_heap_end) {
		kprintf("Kernel has run out of memory!\n");
		return (void *) nil;
	}
	
	knext += size;
	
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
	for (i = l1lo; i < l1hi; i++)
		l1[i] = L1_FAULT;
		
	l1lo = 0;
	l1hi = -1;
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

void
mmu_init(void)
{
	l1lo = 0;
	l1hi = -1;
	
	mmu_empty1();
	
	asm(
		/* Set ttb */
		"ldr r1, =l1			\n"
		"mcr p15, 0, r1, c2, c0, 0	\n"
		/* Set domain mode to manager (for now) */
		"mov r1, #3			\n"
		"mcr p15, 0, r1, c3, c0, 0	\n"
	);
}

