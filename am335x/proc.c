#include "dat.h"
#include "mem.h"

void
proc_init_regs(struct proc *p,
	void (*func)(void *), void *arg)
{
	uint32_t *stack;
	
	p->regs.psr = 16; /* user mode */

	/* Add 4 to address because irq must exit with subs pc, lr, #4 */
	p->regs.pc = (uint32_t) func + 4;
	
	p->regs.regs[0] = (uint32_t) arg; /* Set r0 to arg */
	
	/* Make a page for the stack at the top of memory. */
	p->page = make_page((void *) (0xa0000000 - 1 - PAGE_SIZE));
	stack = kmalloc(sizeof(uint32_t) * 512);
//	p->page->l2[255] = 
//		((uint32_t) stack & PAGE_MASK) | (((1 << 8) - 1) << 4) | L2_SMALL;
	/* Point to top of memory. */
	p->regs.sp = (uint32_t) (stack + 511);
	
	/* Make a page for everything else at the bottom of memory. */
//	p->page->next = make_page((void *) 0);
}
