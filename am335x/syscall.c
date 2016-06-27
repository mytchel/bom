#include "dat.h"
#include "../port/com.h"

void
forkret();

void
forklabel(struct proc *p, struct ureg *ureg)
{
	struct ureg *nreg;

	p->label.sp = (uint32_t) ((uint8_t *) p->kstack + KSTACK - sizeof(struct ureg));
	p->label.pc = (uint32_t) &forkret;
	
	nreg = (struct ureg *) p->label.sp;
	memmove(nreg, ureg, sizeof(struct ureg));
	nreg->regs[0] = 0;
}

void
syscall(struct ureg *ureg)
{
	int ret;
	unsigned int sysnum;

	ret = -1;
	sysnum = (unsigned int) ureg->regs[0];
	
	current->ureg = ureg;	
	if (sysnum < NSYSCALLS) {
		va_list args = (va_list) ureg->sp;
		ret = syscalltable[sysnum](args);
	}
	
	ureg->regs[0] = ret;
}
