#include "dat.h"

void
procinitstack(struct proc *p)
{
	uint32_t *stack;
	stack = kmalloc(sizeof(uint32_t) * 512);
	
	p->label.sp = (uint32_t) (stack + 511 - 14);
}
