#include "types.h"
#include "../include/com.h"

#define HEAP_SIZE 1000000000

static uint8_t heap[HEAP_SIZE];

static uint8_t *next;

void
memory_init(void)
{
	next = heap;
}

void *
kmalloc(size_t size)
{
	void *place = next;

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
