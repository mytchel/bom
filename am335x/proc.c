#include "types.h"
#include "proc.h"

void
proc_init_regs(struct proc_machine *p,
	void (*func)(void *), void *arg)
{
	p->psr = 16; /* user mode */
	p->sp = (uint32_t) (&p->stack[STACK_SIZE]);
	
	/* Add 4 to address because irq must exit with subs pc, lr, #4 */
	p->pc = (uint32_t) func + 4;
	
	/* Set lr so when first starting start_proc can access it 
	 * after dropping to user mode. */
	p->lr = (uint32_t) func;

	p->regs[0] = (uint32_t) arg; /* Set r0 to arg */
}
