#include "dat.h"
#include "mem.h"
#include "../port/com.h"

extern void *_kernel_bin_start;
extern void *_kernel_bin_end;
extern void *_kernel_heap_start;
extern void *_kernel_heap_end;

static uint8_t *next;

static uint32_t l1[4096] __attribute__((__aligned__(16*1024)));

void
memory_init(void)
{
	kprintf("bin_start = 0x%h\n", (uint32_t) &_kernel_bin_start);
	kprintf("bin_end   = 0x%h\n", (uint32_t) &_kernel_bin_end);

	kprintf("heap_start = 0x%h\n", (uint32_t) &_kernel_heap_start);
	kprintf("heap_end   = 0x%h\n", (uint32_t) &_kernel_heap_end);
	
	next = (uint8_t *) &_kernel_heap_start;

	mmu_init();
	
	mmu_enable();
}

struct page *
make_page(void *va)
{
	struct page *p;
	int i;
	
	p = kmalloc(sizeof(struct page));
	if (p == nil) {
		return nil;
	}
	
	for (i = 0; i < 256; i++) {
		p->l2[i] = L2_FAULT;
	}
	
	p->va = va;
	p->next = nil;
	
	return p;
}

void
free_page(struct page *p)
{
	int i;
	void *phys;
	
	for (i = 0; i < 256; i++) {
		if (p->l2[i] != L2_FAULT) {
			phys = (void *) (p->l2[i] & PAGE_MASK);
			kfree(phys);
		}
	}
	
	kfree(p);
}

void
mmu_switch(struct proc *p)
{
	struct page *page;
	uint32_t i;
	
	mmu_disable();	
	mmu_empty1();
	
	for (page = p->page; page != nil; page = page->next) {
		i = ((uint32_t) page->va) >> 20;
		l1[i] = ((uint32_t) page->l2) | L1_COARSE;
	}

	mmu_invalidate();
	mmu_enable();	
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

	/* Map kernel memory */	
	mmu_imap_section((uint32_t) &_kernel_bin_start, 
		(uint32_t) &_kernel_bin_end);

	/* Direct io map, for now. */
	mmu_imap_section(0x40000000, 0x4A400000);
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

