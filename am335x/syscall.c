#include "dat.h"
#include "../port/com.h"

void
forkret();

void
forklabel(struct proc *p, struct ureg *ureg)
{
	struct ureg *nreg;
	
	p->label.sp = (uint32_t) p->kstack + KSTACK - sizeof(ureg);
	p->label.pc = (uint32_t) &forkret;
	
	nreg = (struct ureg *) p->label.sp;
	memmove(nreg, ureg, sizeof(struct ureg));
	nreg->regs[0] = 0;
}

int
syscall(struct ureg *ureg)
{
	int i;
	kprintf("syscall!\n");
	
	for (i = 0; i < 13; i++)
		kprintf("r%i = 0x%h\n", i, ureg->regs[i]);
	kprintf("sp = 0x%h\n", ureg->sp);
	kprintf("lr = 0x%h\n", ureg->lr);
	kprintf("pc = 0x%h\n", ureg->pc);
	kprintf("psr = 0b%b\n", ureg->psr);
	
	
	
	return -1;
}
