#include "types.h"
#include "../include/com.h"

extern uint32_t *_kernel_heap_start;
extern uint32_t *_kernel_heap_end;

static uint8_t *next;

void
memory_init(void)
{
	kprintf("heap_start = 0x%h\n", (uint32_t) _kernel_heap_start);
	kprintf("heap_end   = 0x%h\n", (uint32_t) _kernel_heap_end);
	
	next = (uint8_t*) _kernel_heap_start;
}

void *
kmalloc(size_t size)
{
	void *place = next;

	if (next + size >= (uint8_t*) _kernel_heap_end) {
		kprintf("Kernel has run out of memory!\n");
		return (void *) nil;
	}
	
	next += size;
	
	return place;
}

void
kfree(void *ptr)
{

}
