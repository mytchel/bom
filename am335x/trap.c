#include "dat.h"
#include "trap.h"
#include "fns.h"
#include "mem.h"
#include "../port/com.h"

void
trap(struct ureg *ureg)
{
	uint32_t fsr;
	void *addr;
	struct segment *seg;
	struct page *pg;
	
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
		
		addr = (void *) (ureg->pc - 4);
		
		kprintf("at addr 0x%h\n", (uint32_t) addr);
		
		seg = current->segs[SEG_text];
		
		if (addr >= seg->base && addr < seg->top) {
			for (pg = seg->pages; pg != nil; pg = pg->next) {
				if (pg->va <= addr && pg->va + PAGE_SIZE > addr) {
					kprintf("Found page to map\n");
					mmuputpage(pg);
					break;
				}
			}
		} else {
			kprintf("%i not allowed to access 0x%h\n", 
				current->pid, (uint32_t) addr);
		}
		
		ureg->pc = ureg->pc - 4;
		
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
