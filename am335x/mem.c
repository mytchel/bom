#include "dat.h"
#include "fns.h"
#include "../port/com.h"

static void
initheap(void *, size_t);

static void
addpages(void *, void *);

struct fblock {
	struct fblock *next;
	uint32_t size;
};

struct page_ref {
	void *addr;
	struct page_ref *next;
};

extern uint32_t *_heap_start;
extern uint32_t *_heap_end;
extern uint32_t *_ram_start;
extern uint32_t *_ram_end;
extern uint32_t *_kernel_start;
extern uint32_t *_kernel_end;

static struct fblock heap;
static struct page_ref *pages;

void
initmemory(void)
{
	uint32_t ram_size, heap_size;

	ram_size = (uint32_t) &_ram_end - (uint32_t) &_ram_start;
	heap_size = (uint32_t) &_heap_end - (uint32_t) &_heap_start;

	kprintf("ram size      = %i MB\n", ram_size / 1024 / 1024);
	kprintf("heap size     = %i MB\n", heap_size / 1024 / 1024);

	initheap(&_heap_start, heap_size);
	
	pages = nil;
	addpages((void *) &_ram_start, (void *) &_kernel_start);
	addpages((void *) &_kernel_end, (void *) &_ram_end);
	
	initmmu();
	
	imap(&_ram_start, &_ram_end, AP_RW_NO);
	imap((void *) 0x40000000, (void *) 0x4A400000, AP_RW_RW);
	
	mmuenable();
}

void
initheap(void *heap_start, size_t size)
{
	heap.size = 0;
	heap.next = (struct fblock *) heap_start;
	heap.next->size = size;
	heap.next->next = nil;
}

void
addpages(void *start, void *end)
{
	struct page_ref *first, *p;
	
	start = (void *) ((uint32_t) start & PAGE_MASK);
	
	first = kmalloc(sizeof(struct page_ref));
	p = first;
	while (start < end) {
		p->next = kmalloc(sizeof(struct page_ref));
		p = p->next;
		p->addr = start;
		start += PAGE_SIZE;
	}
	
	p->next = pages;
	
	pages = first->next;
	kfree(first, sizeof(struct page_ref));
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
		if ((void *) b->next > ptr) {
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
	struct page_ref *r;

	p = kmalloc(sizeof(struct page));
	if (p == nil)
		return nil;
		
	r = pages;
	pages = pages->next;
	
	p->pa = r->addr;
	p->va = va;
	p->size = PAGE_SIZE;
	p->next = nil;
	
	kfree(r, sizeof(struct page_ref));

	return p;
}

void
freepage(struct page *p)
{
	struct page_ref *r;
	
	r = kmalloc(sizeof(struct page_ref));
	r->addr = p->pa;
	r->next = pages;
	pages = r;
	
	kfree(p, sizeof(struct page));
}
