#include "dat.h"
#include "mem.h"
#include "p_modes.h"

void
proc_init_regs(struct proc *p,
	void (*func)(void *), void *arg)
{
	uint32_t *stack;
	
	p->regs.psr = MODE_USR;
	p->regs.pc = (uint32_t) func;
	p->regs.regs[0] = (uint32_t) arg; /* Set r0 to arg */
	
	p->regs.ksp = (uint32_t) &(p->stack[KSTACK_SIZE - 1]);
	
	stack = kmalloc(sizeof(uint32_t) * 512);
	p->regs.sp = (uint32_t) (stack + 511);
}
