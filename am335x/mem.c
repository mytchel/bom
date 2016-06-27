#include "dat.h"
#include "mem.h"
#include "../port/com.h"

extern uint32_t *_kernel_heap_start;
extern uint32_t *_kernel_heap_end;
extern uint32_t *_ram_start;
extern uint32_t *_ram_end;
extern uint32_t *_kernel_start;
extern uint32_t *_kernel_end;

struct fblock heap, page;

void
memoryinit(void)
{
	uint32_t ram_size, npages;
	
	kprintf("kernel_start  = 0x%h\n", &_kernel_start);
	kprintf("kernel_end    = 0x%h\n", &_kernel_end);
	kprintf("heap_start    = 0x%h\n", &_kernel_heap_start);
	kprintf("heap_end      = 0x%h\n", &_kernel_heap_end);
	kprintf("ram_start     = 0x%h\n", &_ram_start);
	kprintf("ram_end       = 0x%h\n", &_ram_end);
	
	ram_size = (uint32_t) &_ram_end - (uint32_t) &_ram_start;
	npages = ram_size / PAGE_SIZE;
	
	kprintf("ram           = 0x%h (0x%h)\n", ram_size, npages);

	pageinit();

	heapinit();
	
	mmuinit();
	
	/* Kernel memory */
	imap((uint32_t) &_kernel_start,
		(uint32_t) &_kernel_end);
	
	/* IO memory */
	imap(0x40000000, 0x4A400000);
	
	mmuenable();
}

int
heapinit(void)
{
	heap.size = 0;
	heap.next = (struct fblock *) &_kernel_heap_start;
	heap.next->size = (uint32_t) &_kernel_heap_end - (uint32_t) &_kernel_heap_start;
	heap.next->next = nil;
	
	return 0;
}

int
pageinit(void)
{
	page.size = 0;
	page.next = (struct fblock *) &_ram_start;
	page.next->size = (uint32_t) &_kernel_start - (uint32_t) &_ram_start;
	
	page.next->next = (struct fblock *) &_kernel_end;
	page.next->next->size = (uint32_t) &_ram_end - (uint32_t) &_kernel_end;
	page.next->next->next = nil;

	return 0;
}

static void *
getblock(struct fblock *start, uint32_t size)
{
	struct fblock *b, *n;
	void *block;
	
	for (b = start; b->next != nil; b = b->next) {
		if (b->next->size >= size) {
			break;
		}
	}
	
	if (b->next == nil) {
		kprintf("Kernel has run out of memory!\n");
		return (void *) nil;
	}
	
	block = (void *) b->next;
	if (b->next->size > size) {
		kprintf("breaking up free block 0x%h\n", (uint32_t) b->next);
		n = (struct fblock *) ((uint32_t) b->next + size);
		n->size = b->next->size - size;
		n->next = b->next->next;
		b->next = n;
	} else {
		kprintf("removing free block 0x%h\n", (uint32_t) b->next);
		b->next = b->next->next;
	}
	
	kprintf("allocated 0x%h (0x%h)\n", (uint32_t) block, size);
	return block;
	
}

static void
freeblock(struct fblock *start, void *ptr, uint32_t size)
{
	struct fblock *b;
	uint32_t nsize;
	
	kprintf("freeing 0x%h\n", (uint32_t) ptr);
	
	for (b = start; b->next != nil; b = b->next) {
		if ((void *) b->next->next > ptr) {
			break;
		}
	}
	
	if (b->next == nil) {
		kprintf("0x%h is not a valid pointer?\n", (uint32_t) ptr);
		return;
	}
	
	if (b->next->next != nil &&
		(ptr == (void *) ((uint32_t) b->next + b->next->size)) &&
		(ptr + size == (void *) ((uint32_t) b->next->next))) {
		
		/* Released on boundary of both previous and next. */
		kprintf("released on both previous and next\n");
		
		b->next->size += size + b->next->next->size;
		b->next->next = b->next->next->next;
	
	} else if (ptr == (void *) ((uint32_t) b->next + b->next->size)) {
		/* Released on boundary of previous. */
		kprintf("released on previous\n");
		b->next->size += size;
		
	} else if (ptr + size == (void *) ((uint32_t) b->next->next)) {
		/* Released on boundary of next. */
		kprintf("released on next\n");
		nsize = b->next->next->size + size;
		b->next = (struct fblock *) ((uint32_t) b->next->next - size);
		b->next->size = nsize;
	}
}

void *
kmalloc(size_t size)
{
	return getblock(&heap, size);
}

void
kfree(void *ptr)
{
	kprintf("Not yet implimented\n");
}

void *
getpage(void)
{
	return getblock(&page, PAGE_SIZE);
}

void
freepage(void *pa)
{
	freeblock(&page, pa, PAGE_SIZE);
}
