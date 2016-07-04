#include "dat.h"

struct block {
	size_t size;
	struct block *next;
};

static struct block *heap;

void
initheap(void *heap_start, size_t size)
{
	heap = (struct block *) heap_start;
	heap->size = size - sizeof(size_t);
	heap->next = nil;
}

void *
kmalloc(size_t size)
{
	struct block *b, *n, *p;
	void *block;

	p = nil;
	for (b = heap; b != nil; p = b, b = b->next) {
		if (b->size >= size) {
			break;
		}
	}
	
	if (b == nil) {
		kprintf("Kernel has run out of memory!\n");
		return (void *) nil;
	}

	block = (void *) ((reg_t) b + sizeof(size_t));
	
	if (b->size > size) {
		n = (struct block *) ((reg_t) block + size);
		n->size = b->size - size - sizeof(size_t);
		n->next = b->next;
	} else {
		n = b->next;
	}
	
	/* Set size of allocated block (so kfree can find it) */
	b->size = size;
	
	if (p == nil) {
		heap = n;
	} else {
		p->next = n;
	}

	return block;
}

void
kfree(void *ptr)
{
	struct block *b, *p;

	b = (struct block *) ((reg_t) ptr - sizeof(size_t));

	if (b < heap) {
		if ((reg_t) b + sizeof(size_t) + b->size == (reg_t) heap) {
			/* block lines up with start of heap. */
			b->size += sizeof(size_t) + heap->size;
			b->next = heap->next;
		} else {
			b->next = heap;
		}
		
		heap = b;
		return;
	}

	for (p = heap; p != nil; p = p->next) {
		if ((void *) p->next > ptr) {
			break;
		}
	}
	
	if (p == nil) {
		kprintf("Are you sure 0x%h is from the heap?\n", ptr);
		return;
	}
	
	if (p->next == nil) {
		/* b is at end of list, append. */
		
		b->next = nil;
		p->next = b;

	} else if (((reg_t) b + sizeof(size_t) + b->size == (reg_t) p->next)
		&& ((reg_t) p + p->size == (reg_t) b)) {
		/* b lines up with p and p->next, join them all. */
		
		p->size += sizeof(size_t) * 2 + b->size + p->next->size;
		p->next = p->next->next;
		
	} else if ((reg_t) b + sizeof(size_t) + b->size == (reg_t) p->next) {
		/* b lines up with p->next, join them. */
		
		b->size += sizeof(size_t) + p->next->size;
		b->next = p->next->next;
		p->next = b;

	} else if ((reg_t) p + p->size == (reg_t) b) {
		/* b lines up with end of p, join them. */

		p->size += sizeof(size_t) + b->size;

	} else {
		/* b is somewhere between p and p->next. */

		b->next = p->next;
		p->next = b;
	}
}

/* This can go here for now. */

void
memmove(void *new, const void *old, size_t l)
{
	size_t i;
	char *n;
	const char *o;
	
	n = new;
	o = old;
	
	for (i = 0; i < l; i++)
		n[i] = o[i];
}
