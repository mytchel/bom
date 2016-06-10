#include "types.h"
#include "../include/com.h"

#define HEAP_SIZE 100000000

static uint8_t heap[HEAP_SIZE];

static uint8_t *next;

void
memory_init(void)
{
	kprintf("kmalloc_init\n");
		
	next = heap;
}

void *
kmalloc(size_t size)
{
	void *place = next;

	kprintf("attempt to allocate 0x%h bytes\n", size);
	kprintf("next: 0x%h\n", next);

	if (next + size > heap + HEAP_SIZE) {
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
