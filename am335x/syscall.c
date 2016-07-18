#include "dat.h"
#include "fns.h"

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
forkfunc(struct proc *p, int (*func)(void))
{
	int i;
	for (i = 0; i < 8; i++)
		p->label.regs[i] = 0;
	p->label.sp = (uint32_t) p->kstack + KSTACK;
	p->label.pc = (uint32_t) func;
}

void
forkchild(struct proc *p, struct ureg *ureg)
{
	struct ureg *nreg;

	p->label.sp = (uint32_t) p->kstack + KSTACK - sizeof(struct ureg);
	p->label.pc = (uint32_t) &uregret;
	
	nreg = (struct ureg *) p->label.sp;
	memmove(nreg, ureg, sizeof(struct ureg));
	nreg->regs[0] = 0;
}

void
syscall(struct ureg *ureg)
{
	unsigned int sysnum;

	sysnum = (unsigned int) ureg->regs[0];

	ureg->regs[0] = -1;
	current->ureg = ureg;
	
	if (sysnum < NSYSCALLS) {
		va_list args = (va_list) ureg->sp;
		ureg->regs[0] = syscalltable[sysnum](args);
	}
	
}

