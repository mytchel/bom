#include "dat.h"
#include "mem.h"
#include "../port/com.h"

extern void *_kernel_heap_start;
extern void *_kernel_heap_end;

struct fblock fblocks;

void
memory_init(void)
{
	heap_init();
		
	mmu_init();
	
	mmu_enable();
}

void
heap_init(void)
{
	kprintf("heap_start = 0x%h\n", &_kernel_heap_start);
	kprintf("heap_end   = 0x%h\n", &_kernel_heap_end);
	
	fblocks.next = (struct fblock *) &_kernel_heap_start;
	fblocks.size = 0;
	
	fblocks.next->next = nil;
	fblocks.next->size = (uint32_t) &_kernel_heap_end - (uint32_t) &_kernel_heap_start;
}

void *
kmalloc(size_t size)
{
	struct fblock *b, *p;
	void *block;
	
	p = &fblocks;
	for (b = p->next; b != nil; p = b, b = b->next) {
		if (b->size > size) {
			break;
		}
	}
	
	if (b == nil) {
		kprintf("Kernel has run out of memory!\n");
		return (void *) nil;
	}
	
	block = (void *) b;
	
	p->next = (struct fblock *) ((uint32_t) b + size);
	p->next->next = b->next;
	p->next->size = b->size - size;
	
	return block;
}

void
kfree(void *ptr)
{
	struct fblock *p, *n;
	
	for (p = fblocks.next; p != nil; p = p->next)
		if ((void *) p->next > ptr)
			break;
	
	if (p == nil) {
		kprintf("Not sure whats happened\n");
		return;
	}
	
	n = p->next;
	
	p->next = (struct fblock *) ptr;
	p->next->next = n;
	p->next->size = (uint32_t) n - (uint32_t) ptr;
}

