#ifndef __DAT_H
#define __DAT_H

#include "../include/types.h"

#define MAX_PROCS	512
#define KSTACK		4028

#define USTACK_TOP	(0x20000000 & PAGE_MASK)
#define USTACK_SIZE	PAGE_SIZE
#define UTEXT		0

#define MAX_MEM_SIZE	1024 * 1024 *32

struct label {
	uint32_t regs[9];
	uint32_t sp, pc;
};

struct ureg {
	uint32_t regs[13];
	uint32_t sp, lr;
	uint32_t type, psr, pc;
};

#include "../port/portdat.h"

#endif
