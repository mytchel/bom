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

struct page {
	uint32_t l2[256] __attribute__((__aligned__(1024)));
	void *va;
	struct page *next;
};

struct proc_regs {
	uint32_t psr, sp, pc, lr;
	uint32_t regs[13];
};

#include "../port/port_dat.h"

#endif
