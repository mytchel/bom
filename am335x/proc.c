#include "dat.h"
#include "mem.h"
#include "p_modes.h"
#include "../include/stdarg.h"

void
proc_init_stack(struct proc *p)
{
	uint32_t *stack;
	
	p->regs.ksp = (uint32_t) 
		(p->stack + KSTACK_SIZE - sizeof(uint32_t));
	
	/* This will suffice for now. */
	stack = kmalloc(sizeof(uint32_t) * STACK_SIZE);
	p->regs.sp = (uint32_t) (stack + STACK_SIZE - 1);
}

void
proc_init_regs(struct proc *p,
	void (*func)(void), ...)
{
	int i;
	va_list ap;
	
	p->regs.psr = MODE_USR;
	p->regs.pc = (uint32_t) func;
	
	va_start(ap, func);
	for (i = 0; i < 5; i++) {
		p->regs.regs[i] = va_arg(ap, uint32_t);
	}
	va_end(ap);
}
