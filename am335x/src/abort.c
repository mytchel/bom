#include <io.h>
#include <com.h>
#include <machine/types.h>

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
