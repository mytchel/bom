#include "types.h"
#include "mem.h"
#include "../include/com.h"

extern void *_kernel_bin_start;
extern void *_kernel_bin_end;

extern void *_kernel_heap_start;
extern void *_kernel_heap_end;

static uint8_t *next;

static uint32_t mmu_ttb[4096];

void
mmu_init(void)
{
	int i;
	
	kprintf("mmu_init\n");	
	
	for (i = 0; i < 4096; i++)
		mmu_ttb[i] = L1_FAULT;
	
	asm(
		"ldr r0, =mmu_ttb		\n"
		"mcr p15, 0, r0, c2, c0, 0	\n"
	);
}


void
mmu_enable(void)
{
	kprintf("mmu_enable\n");
	asm(
		/* Invalidate tlb */
		"mcr p15, 0, r1, c8, c7, 0	\n"
		/* Enable mmu */
		"mrc p15, 0, r0, c1, c0, 0	\n"
		"orr r0, r0, #1			\n"
		"mcr p15, 0, r0, c1, c0, 0	\n"
	);
}

void
mmu_disable(void)
{
	asm(
		"mrc p15, 0, r0, c1, c0, 0	\n"
		"bic r0, r0, #1			\n"
		"mcr p15, 0, r0, c1, c0, 0	\n"
	);
}

void
mmu_map_page(void *phys, void *vert, size_t npages, uint8_t perms)
{
	int i;
	uint32_t virt_a, phys_a;
	
	virt_a = (uint32_t) vert;
	phys_a = (uint32_t) phys;
	
	for (i = 0; i < npages; i++) {
		id = virt_a >> 20;
		
		mmu_ttb[id] = phys_a
		
		virt_a += PAGE_SIZE;
		phys_a += PAGE_SIZE;
	}
	
	/* Invalidate tlb */	
	asm("mcr p15, 0, r1, c8, c7, 0");
}

void
memory_init(void)
{
	size_t kbounds, kbounde;
	
	kprintf("bin_start = 0x%h\n", (uint32_t) &_kernel_bin_start);
	kprintf("bin_end   = 0x%h\n", (uint32_t) &_kernel_bin_end);

	kprintf("heap_start = 0x%h\n", (uint32_t) &_kernel_heap_start);
	kprintf("heap_end   = 0x%h\n", (uint32_t) &_kernel_heap_end);
	
	next = (uint8_t *) &_kernel_heap_start;
	
	mmu_init();

	/* Map kernel memory */	
	kbounds = PAGE_ALIGN((uint32_t) &_kernel_bin_start) >> PAGE_SHIFT;
	kbounde = PAGE_ALIGN((uint32_t) &_kernel_bin_end) >> PAGE_SHIFT;

	mmu_map_page(&_kernel_bin_start, &_kernel_bin_start, 
		kbounde - kbounds, MMU_AP_RW_NO);

	mmu_enable();
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
