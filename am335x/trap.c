#include "dat.h"
#include "trap.h"
#include "fns.h"
#include "../port/com.h"

void
trap(struct ureg *ureg)
{
	kprintf("trap type %i\n", ureg->type);

	mmudisable(); /* mmuenable called on return in uregret. */
	
	switch(ureg->type) {
	case ABORT_INTERRUPT:
		kprintf("irq\n");
		break;
	case ABORT_INSTRUCTION:
		kprintf("undefined instruction\n");
		break;
	case ABORT_PREFETCH:
		kprintf("prefetch abort\n");
		break;
	case ABORT_DATA:
		kprintf("data abort\n");
		break;
	}
}
