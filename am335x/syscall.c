#include "dat.h"
#include "fns.h"
#include "../port/com.h"

void
dumpregs(struct ureg *r)
{
	int i;
	for (i = 0; i < 13; i++)
		kprintf("r%i  = 0x%h\n", i, r->regs[i]);
	kprintf("sp   = 0x%h\n", r->sp);
	kprintf("lr   = 0x%h\n", r->lr);
	kprintf("type = %i\n", r->type);
	kprintf("pc   = 0x%h\n", r->pc);
	kprintf("psr  = 0b%b\n", r->psr);
}

void
forklabel(struct proc *p, struct ureg *ureg)
{
	struct ureg *nreg;

	p->label.sp = (uint32_t) ((uint8_t *) p->kstack + KSTACK - sizeof(struct ureg));
	p->label.pc = (uint32_t) &uregret;
	
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
	
	kprintf("syscall %i\n", sysnum);
	
	current->ureg = ureg;	
	if (sysnum < NSYSCALLS) {
		va_list args = (va_list) ureg->sp;
		ret = syscalltable[sysnum](args);
	}
	
	ureg->regs[0] = ret;
}

