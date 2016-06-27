#ifndef __DAT_H
#define __DAT_H

#include "types.h"

#define PAGE_SHIFT 	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))
#define PAGE_ALIGN(x) 	(((x) + PAGE_SIZE - 1) & PAGE_MASK)

#define PAGE_FREE	0
#define PAGE_USED	1

struct label {
	uint32_t sp, pc;
};

struct user_regs {
	uint32_t psr;
	uint32_t sp;
	uint32_t lr;
	uint32_t regs[13];
};

#include "../port/portdat.h"

#endif
