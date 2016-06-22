#ifndef __DAT_H
#define __DAT_H

typedef	signed char		int8_t;
typedef	unsigned char		uint8_t;
typedef	short			int16_t;
typedef	unsigned short		uint16_t;
typedef	int			int32_t;
typedef	unsigned int		uint32_t;
typedef	long long		int64_t;
typedef	unsigned long long	uint64_t;

typedef double			double_t;
typedef float			float_t;
typedef long			ptrdiff_t;
typedef	unsigned long		size_t;
typedef	long			ssize_t;

typedef unsigned int		reg_t;

#define PAGE_SHIFT 	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))
#define PAGE_ALIGN(x) 	(((x) + PAGE_SIZE - 1) & PAGE_MASK)

struct proc_regs {
	uint32_t cpsr, spsr;
	uint32_t ksp;
	uint32_t sp, lr;
	uint32_t regs[13];
	uint32_t pc;
};

#include "../port/port_dat.h"

#endif
