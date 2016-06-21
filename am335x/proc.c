#include "dat.h"

void
proc_init_regs(struct proc *p,
	void (*func)(void *), void *arg)
{
	p->regs.psr = 16; /* user mode */
	p->regs.sp = (uint32_t) (&p->stack[STACK_SIZE]);
	
	/* Add 4 to address because irq must exit with subs pc, lr, #4 */
	p->regs.pc = (uint32_t) func + 4;
	
	p->regs.regs[0] = (uint32_t) arg; /* Set r0 to arg */
}
