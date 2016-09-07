#include "head.h"
#include "fns.h"

void
dumpregs(struct ureg *r)
{
	int i;
	for (i = 0; i < 13; i++)
		printf("r%i  = 0x%h\n", i, r->regs[i]);
	printf("sp   = 0x%h\n", r->sp);
	printf("lr   = 0x%h\n", r->lr);
	printf("type = %i\n", r->type);
	printf("pc   = 0x%h\n", r->pc);
	printf("psr  = 0b%b\n", r->psr);
}

void
forkfunc(struct proc *p, int (*func)(void *), void *arg)
{
	int i;
	for (i = 0; i < 8; i++)
		p->label.regs[i] = 0;
	
	p->label.pc = (uint32_t) &forkfunc_loader;
	p->label.sp = (uint32_t) forkfunc_preloader(
		(void *) (p->kstack + KSTACK - sizeof(uint32_t)),
		arg, func);
}

void
forkchild(struct proc *p, struct ureg *ureg)
{
	struct ureg *nreg;

	p->label.pc = (uint32_t) &uregret;
	p->label.sp = (uint32_t) 
		p->kstack + KSTACK - sizeof(uint32_t) - sizeof(struct ureg);
	
	nreg = (struct ureg *) p->label.sp;
	memmove(nreg, ureg, sizeof(struct ureg));
	nreg->regs[0] = 0;
}
