#ifndef __MEM
#define __MEM

#define PAGE_SHIFT 	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))
#define PAGE_ALIGN(x) 	(((x) + PAGE_SIZE - 1) & PAGE_MASK)

#define MMU_AP_UP_UP 0
#define MMU_AP_RW_NO 1
#define MMU_AP_RW_RO 2
#define MMU_AP_RW_RW 3

#define L1_FAULT	0
#define L1_COARSE	1
#define L1_SECTION	2
#define L1_FINE		3

#define L2_FAULT	0
#define L2_LARGE	1
#define L2_SMALL	2
#define L2_TINY		3

void *
kmalloc(size_t size);

void
kfree(void *ptr);

void
mmu_init(void);

void
mmu_enable(void);

void
mmu_disable(void);

void
mmu_map_page(void *phys, void *vert,
	size_t npages, uint8_t perms);
	
void
memory_init(void);

#endif
