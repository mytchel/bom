#include "dat.h"
#include "mem.h"
#include "p_modes.h"
#include "../include/std.h"
#include "../port/com.h"

void
proc_init_stack(struct proc *p)
{
	reg_t *stack;
	
	p->regs.ksp = (reg_t) (&p->stack[KSTACK_SIZE]);
	
	/* This will suffice for now. */
	stack = kmalloc(sizeof(reg_t) * STACK_SIZE);
	p->regs.sp = (reg_t) (stack + STACK_SIZE - 1);
}

void
proc_init_regs(struct proc *p, int (*exit)(int),
	int (*func)(int, void *), int argc, void *arg)
{
	p->regs.psr = MODE_USR;
	p->regs.pc = (reg_t) func;
	p->regs.lr = (reg_t) exit;
	
	p->regs.regs[0] = (reg_t) argc;
	p->regs.regs[1] = (reg_t) arg;
}
