#include "head.h"

struct block {
	size_t size;
	struct block *next;
};

static struct block *heap = nil;

struct block *
growheap(struct block *prev, size_t size)
{
	struct block *b;
		
	b = getmem(&size);
	if (b == nil) {
		return nil;
	} else if (prev == nil) {
		heap = b;
	} else {
		prev->next = b;
	}
	
	b->size = size - sizeof(size_t);
	b->next = nil;
	
	return b;
}

void *
malloc(size_t size)
{
	struct block *b, *n, *p;
	void *block;

	if (size < sizeof(struct block *)) {
		size = sizeof(struct block *);
	}
	
	p = nil;
	for (b = heap; b != nil; p = b, b = b->next) {
		if (b->size >= size) {
			break;
		}
	}
	
	if (b == nil) {
		b = growheap(p, size * 2);
		if (b == nil) {
			printf("Out of memory!\n");
			return nil;
		}
	}

	block = (void *) ((uint8_t *) b + sizeof(size_t));
	
	if (b->size > size + sizeof(struct block)) {
		n = (struct block *) (((uint8_t *) block + size) + 
			(reg_t) ((uint8_t *) block + size) % sizeof(reg_t));

		n->size = b->size - size - sizeof(size_t);
		n->next = b->next;
		
		/* Set size of allocated block (so kfree can find it) */
		b->size = size;
	} else {
		n = b->next;
	}
	
	if (p == nil) {
		heap = n;
	} else {
		p->next = n;
	}
	
	return block;
}

void
free(void *ptr)
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
		printf("Are you sure 0x%h is from the heap?\n", ptr);
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
