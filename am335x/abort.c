#include "io.h"
#include "types.h"
#include "../include/com.h"

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
