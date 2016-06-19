#include "types.h"
#include "mem.h"
#include "../include/com.h"

extern void *_kernel_bin_start;
extern void *_kernel_bin_end;

extern void *_kernel_heap_start;
extern void *_kernel_heap_end;

static uint8_t *next;

static uint32_t mmu_ttb[4096] __attribute__((__aligned__(16* 1024)));
static uint32_t l2[4096][256] __attribute__((__aligned__(1024)));

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
	kbounds = PAGE_ALIGN((uint32_t) &_kernel_bin_start - 0x80000000) >> PAGE_SHIFT;
	kbounde = PAGE_ALIGN((uint32_t) &_kernel_bin_end -   0x80000000) >> PAGE_SHIFT;

	mmu_map_page(&_kernel_bin_start, &_kernel_bin_start, 
		kbounde - kbounds, MMU_AP_RW_NO);

	/* Direct map io map, for now. */
	mmu_map_page((void *) 0x40000000, (void *) 0x40000000,
		(0x4A400000 - 0x40000000) >> PAGE_SHIFT, MMU_AP_RW_NO);

	mmu_enable();
}

void
mmu_init(void)
{
	int i;
	
	kprintf("mmu_init\n");	
	
	for (i = 0; i < 4096; i++)
		mmu_ttb[i] = L1_FAULT;
	
	asm(
		/* Set ttb */
		"ldr r1, =mmu_ttb		\n"
		"mcr p15, 0, r1, c2, c0, 0	\n"
		/* Disable domains ? */
		"mov r1, #3			\n"
		"mcr p15, 0, r1, c3, c0, 0	\n"
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
mmu_map_page(void *phys, void *vert, size_t npages, uint8_t perms)
{
	int i, j, id;
	uint32_t *page, virt_a, phys_a;
	
	virt_a = (uint32_t) vert;
	phys_a = (uint32_t) phys;
	
	kprintf("mmu_map_page 0x%h size 0x%h\n", (uint32_t) phys, npages);
	
	for (i = 0; i < npages; i++) {
		id = virt_a >> 20;
	
		/* If L1 for address is not set, point it to the L2 table. */
		if (mmu_ttb[id] == L2_FAULT) {
			mmu_ttb[id] = (uint32_t) &(l2[id]) | L1_COARSE;
			for (j = 0; j < 256; j++)
				l2[id][j] = L2_FAULT;
		}
		
		/* Pointer to coarse page table. */
		page = (uint32_t *) (mmu_ttb[id] & ~((1 << 10) - 1));
		
		id = (virt_a & 0xff000) >> 12;
		
		page[id] = (phys_a & 0xfffff000) | L2_SMALL | (0xff<<4);

		virt_a += PAGE_SIZE;
		phys_a += PAGE_SIZE;
	}
	
	/* Invalidate tlb */	
	asm("mcr p15, 0, r1, c8, c7, 0");
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
