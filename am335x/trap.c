#include "dat.h"
#include "trap.h"
#include "fns.h"
#include "../port/com.h"

void
trap(struct ureg *ureg)
{
	uint32_t fsr;
	void *addr;
	
	kprintf("trap type %i\n", ureg->type);
	
	switch(ureg->type) {
	case ABORT_INTERRUPT:
		kprintf("irq\n");
		break;
	case ABORT_INSTRUCTION:
		kprintf("undefined instruction\n");
		break;
	case ABORT_PREFETCH:
		kprintf("prefetch abort\n");
		
		addr = (void *) ureg->pc - 4;
		
		kprintf("at addr 0x%h\n", (uint32_t) addr);
		
		break;
	case ABORT_DATA:
		kprintf("data abort\n");
		
		fsr = fsrstatus();
		addr = faultaddr();
		
		kprintf("fsr = 0b%h\n", fsr);
		kprintf("at addr 0x%h\n", (uint32_t) addr);
		
		break;
	}
}
