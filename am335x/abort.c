#include "dat.h"
#include "../port/com.h"

void
abort_handler(uint32_t ptr, uint32_t code)
{
	kprintf("abort hander: ");
	if (code) {
		kprintf("data\n");
	} else {
		kprintf("prefetch\n");
	}
}
