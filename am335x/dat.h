#ifndef __DAT_H
#define __DAT_H

#include "types.h"

#define MAX_PROCS	512
#define KSTACK		4028

#define PAGE_SHIFT 	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))
#define PAGE_ALIGN(x) 	(((x) + PAGE_SIZE - 1) & PAGE_MASK)

struct label {
	uint32_t sp, pc;
};

struct ureg {
	uint32_t regs[13];
	uint32_t sp;
	uint32_t lr;
	uint32_t psr, pc;
};

#include "../port/portdat.h"

#endif
