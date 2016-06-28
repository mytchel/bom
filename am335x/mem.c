#include "dat.h"
#include "mem.h"
#include "../port/com.h"

extern uint32_t *_heap_start;
extern uint32_t *_heap_end;
extern uint32_t *_ram_start;
extern uint32_t *_ram_end;
extern uint32_t *_kernel_start;
extern uint32_t *_kernel_end;

static struct fblock heap;
static struct page *pages;
static struct page *first;

void
memoryinit(void)
{
	uint32_t ram_size, heap_size;
	
	kprintf("kernel_start  = 0x%h\n", &_kernel_start);
	kprintf("kernel_end    = 0x%h\n", &_kernel_end);
	
	ram_size = (uint32_t) &_ram_end - (uint32_t) &_ram_start;
	heap_size = (uint32_t) &_heap_end - (uint32_t) &_heap_start;

	kprintf("ram size      = %i MB\n", ram_size / 1024 / 1024);
	kprintf("heap size     = %i MB\n", heap_size / 1024 / 1024);

	heapinit((void *) &_heap_start, heap_size);
	
	pageinit((void *) &_ram_start, ram_size);
	
	mmuinit();
	
	/* Kernel memory */
	imap((uint32_t) &_kernel_start,
		(uint32_t) &_kernel_end);
	
	/* IO memory */
	imap(0x40000000, 0x4A400000);
}

int
heapinit(void *heap_start, size_t size)
{
	heap.size = 0;
	heap.next = (struct fblock *) heap_start;
	heap.next->size = size;
	heap.next->next = nil;
	
	return 0;
}

int
pageinit(void *ram_start, size_t ram_size)
{
	uint32_t i, npages;
	
	npages = ram_size / PAGE_SIZE;
	
	pages = kmalloc(sizeof(struct page) * npages);
	
	for (i = 0; i < npages; i++) {
		pages[i].pa = (void *) ((uint32_t) ram_start + i * PAGE_SIZE);
		if (i < npages - 1)
			pages[i].next = &pages[i + 1];
		else
			pages[i].next = nil;
	}
	
	/* Don't touch first page, may have vectors. */
	first = pages->next;
	
	return 0;
}

void *
kmalloc(size_t size)
{
	struct fblock *b, *n;
	void *block;
	
	for (b = &heap; b->next != nil; b = b->next) {
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
		n = (struct fblock *) ((uint32_t) b->next + size);
		n->size = b->next->size - size;
		n->next = b->next->next;
		b->next = n;
	} else {
		b->next = b->next->next;
	}
	
	return block;
	
}

void
kfree(void *ptr, size_t size)
{
	struct fblock *b;
	uint32_t nsize;
	
	for (b = &heap; b->next != nil; b = b->next) {
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
		
		b->next->size += size + b->next->next->size;
		b->next->next = b->next->next->next;
	
	} else if (ptr == (void *) ((uint32_t) b->next + b->next->size)) {
		/* Released on boundary of previous. */
		b->next->size += size;
		
	} else if (ptr + size == (void *) ((uint32_t) b->next->next)) {
		/* Released on boundary of next. */
		nsize = b->next->next->size + size;
		b->next = (struct fblock *) ((uint32_t) b->next->next - size);
		b->next->size = nsize;
	}
}

struct page *
newpage(void *va)
{
	struct page *p;

	p = first;
	first = first->next;
	p->va = va;
	
	return p;
}

void
freepage(struct page *p)
{
	p->next = first;
	first = p;
}
