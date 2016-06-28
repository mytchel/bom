#ifndef __DAT_H
#define __DAT_H

#include "types.h"

#define PAGE_SHIFT 	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))
#define PAGE_ALIGN(x) 	(((x) + PAGE_SIZE - 1) & PAGE_MASK)

#define MAX_PROCS	512
#define KSTACK		4028

#define USTACK_TOP	0x20000000
#define USTACK_SIZE	PAGE_SIZE
#define UTEXT		0x00008000

struct label {
	uint32_t sp, pc;
};

struct ureg {
	uint32_t regs[13];
	uint32_t sp, lr;
	uint32_t type, psr, pc;
};

#include "../port/portdat.h"

#endif
