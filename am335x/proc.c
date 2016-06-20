#include "../port/types.h"
#include "../port/proc.h"

void
proc_init_regs(struct proc *p,
	void (*func)(void *), void *arg)
{
	p->machine.psr = 16; /* user mode */
	p->machine.sp = (uint32_t) (&p->stack[STACK_SIZE]);
	
	/* Add 4 to address because irq must exit with subs pc, lr, #4 */
	p->machine.pc = (uint32_t) func + 4;
	
	p->machine.regs[0] = (uint32_t) arg; /* Set r0 to arg */
}
