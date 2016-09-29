/*
 *
 * Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "head.h"

struct block {
  size_t size;
  struct block *next;
};

static struct block *heap;
static struct lock heaplock;

void
initheap(void *start, size_t size)
{
  initlock(&heaplock);
  heap = (struct block *) start;
  heap->size = size - sizeof(size_t);
  heap->next = nil;
}

struct block *
growheap(struct block *prev)
{
  struct page *pg;
  struct block *b;

  pg = newpage(0);
  if (pg == nil) {
    return nil;
  } else if (prev == nil) {
    b = heap = (struct block *) pg->pa;
  } else {
    b = prev->next = (struct block *) pg->pa;
  }
	
  b->size = pg->size - sizeof(size_t);
  b->next = nil;
  return b;
}

void *
malloc(size_t size)
{
  struct block *b, *n, *p;
  void *block;

  if (size == 0)
    return nil;

  size = roundptr(size);

  lock(&heaplock);
  
  p = nil;
  for (b = heap; b != nil; p = b, b = b->next) {
    if (b->size >= size) {
      break;
    }
  }
	
  if (b == nil) {
    b = growheap(p);
    if (b == nil) {
      printf("KERNEL OUT OF MEMORY!\n");
      unlock(&heaplock);
      return nil;
    }
  }

  block = (void *) ((uint8_t *) b + sizeof(size_t));
	
  if (b->size > size + sizeof(size_t) + sizeof(struct block)) {
    n = (struct block *) ((uint8_t *) block + size);
		
    n->size = b->size - size - sizeof(size_t);
    n->next = b->next;
		
    /* Set size of allocated block (so free can find it) */
    b->size = size;
  } else {
    n = b->next;
  }
	
  if (p == nil) {
    heap = n;
  } else {
    p->next = n;
  }

  unlock(&heaplock);
  return block;
}

void
free(void *ptr)
{
  struct block *b, *p;
  bool palign, nalign;

  if (ptr == nil)
    return;

  b = (struct block *) ((reg_t) ptr - sizeof(size_t));

  lock(&heaplock);
  
  if (b < heap) {
    if ((reg_t) b + sizeof(size_t) + b->size == (reg_t) heap) {
      /* block lines up with start of heap. */
      b->size += sizeof(size_t) + heap->size;
      b->next = heap->next;
    } else {
      b->next = heap;
    }
		
    heap = b;
    unlock(&heaplock);
    return;
  }

  for (p = heap; p != nil && p->next != nil; p = p->next) {
    if ((void *) p->next > ptr && ptr < (void *) p->next->next) {
      break;
    }
  }
	
  if (p == nil) {
    printf("Are you sure 0x%h is from the heap?\n", ptr);
    unlock(&heaplock);
    return;
  } else if (p->next == nil) {
    /* b is at end of list, append. */
    b->next = nil;
    p->next = b;
    unlock(&heaplock);
    return;
  }
	
  palign = (uint8_t *) p + p->size == (uint8_t *) b;
  nalign = (uint8_t *) b + b->size == (uint8_t *) p->next;
	
  if (palign && nalign) {
    /* b lines up with p and p->next, join them all. */
		
    p->size += sizeof(size_t) * 2 + b->size + p->next->size;
    p->next = p->next->next;
		
  } else if (nalign) {
    /* b lines up with p->next, join them. */
		
    b->size += sizeof(size_t) + p->next->size;
    b->next = p->next->next;
    p->next = b;

  } else if (palign) {
    /* b lines up with end of p, join them. */
    p->size += sizeof(size_t) + b->size;

  } else {
    /* b is somewhere between p and p->next. */
    b->next = p->next;
    p->next = b;
  }

  unlock(&heaplock);
}
