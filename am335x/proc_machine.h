#ifndef __PROC_MACHINE
#define __PROC_MACHINE

struct page {
	uint32_t l2[256] __attribute__((__aligned__(1024)));
	void *va;
	struct page *next;
};

struct proc_machine {
	uint32_t psr, sp, pc, lr;
	uint32_t regs[13];
};

#endif
