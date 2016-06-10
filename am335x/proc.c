#include "types.h"
#include "proc.h"

void
proc_init_regs(struct proc_machine *p,
	void (*func)(void *), void *arg)
{
	p->psr = 16; /* user mode */

	p->sp = (uint32_t) (p->stack + STACK_SIZE - 14);
//	t->stack[t->sp] = (uint32_t) arg;
	
	p->lr = (uint32_t) func;
}
