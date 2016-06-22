#include "dat.h"
#include "abort.h"
#include "../port/com.h"

void
abort_handler(uint32_t code)
{
	kprintf("abort hander: ");
	switch (code) {
	case ABORT_INSTRUCTION:
		kprintf("instruction\n");
		break;
	case ABORT_PREFETCH:
		kprintf("prefetch\n");
		break;
	case ABORT_DATA:
		kprintf("data\n");
		break;
	}

	kprintf("killing %i\n", current->pid);	
	proc_remove(current);
	schedule();
}
