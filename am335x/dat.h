#ifndef __DAT_H
#define __DAT_H

#include "types.h"

#define PAGE_SHIFT 	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))
#define PAGE_ALIGN(x) 	(((x) + PAGE_SIZE - 1) & PAGE_MASK)

struct proc_regs {
	uint32_t psr;
	uint32_t ksp;
	uint32_t sp, lr, pc;
	uint32_t regs[13];
};

#include "../port/port_dat.h"

#endif
