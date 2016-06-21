#ifndef __MEM_H
#define __MEM_H

#define PAGE_SHIFT 	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))
#define PAGE_ALIGN(x) 	(((x) + PAGE_SIZE - 1) & PAGE_MASK)

#define SECTION_SHIFT 		20
#define SECTION_SIZE		(1UL << SECTION_SHIFT)
#define SECTION_MASK		(~(SECTION_SIZE - 1))
#define SECTION_ALIGN(x)	(((x) + SECTION_SIZE - 1) & SECTION_MASK)

#define L1_FAULT	0
#define L1_COARSE	1
#define L1_SECTION	2
#define L1_FINE		3

#define L2_FAULT	0
#define L2_LARGE	1
#define L2_SMALL	2
#define L2_TINY		3

void
mmu_invalidate(void);

void
mmu_enable(void);

void
mmu_disable(void);

void
mmu_imap_section(uint32_t start, uint32_t end);
	
void
memory_init(void);

#endif
