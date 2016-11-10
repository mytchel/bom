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

static struct lock heaplock = {0};
static struct block *heap = nil;
static struct pagel *pages = nil;

void
heapinit(void *start, size_t size)
{
  heap = (struct block *) start;
  heap->size = size - sizeof(size_t);
  heap->next = nil;
}

struct block *
growheap(struct block *prev)
{
  struct pagel *pl;
  struct page *pg;
  struct block *b;

  pg = getrampage();
  if (pg == nil) {
    return nil;
  }

  pl = (struct pagel *) pg->pa;
  
  pl->p = pg;
  pl->rw = true;
  pl->c = true;
  pl->next = pages;
  pages = pl;

  b = (struct block *) ((uint8_t *) pg->pa + sizeof(struct pagel));
  
  if (prev == nil) {
    heap = b;
  } else {
    prev->next = b;
  }
	
  b->size = PAGE_SIZE - sizeof(struct pagel) - sizeof(size_t);
  b->next = nil;

  return b;
}

void *
malloc(size_t size)
{
  struct block *b, *n, *p;
  void *ptr;

  if (size == 0)
    return nil;

  size = roundptr(size);

  if (size > PAGE_SIZE - sizeof(struct pagel) - sizeof(size_t)) {
    printf("%i trying to malloc something too large %i\n",
	   up->pid, size);

    setintr(INTR_OFF);
    procexit(up, ENOMEM);
    schedule();

    return nil;
  }

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
      panic("KERNEL OUT OF MEMORY!\n");
      return nil;
    }
  }

  ptr = (void *) ((uint8_t *) b + sizeof(size_t));
	
  if (b->size > size + sizeof(size_t) + sizeof(struct block)) {
    n = (struct block *) ((uint8_t *) b + sizeof(size_t) + size);
		
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

  return ptr;
}

void
free(void *ptr)
{
  struct pagel *pl, *pp;
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

    goto done;
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
  }
	
  palign = (reg_t) p + sizeof(size_t) + p->size == (reg_t) b;

  if (p->next) {
    nalign = (reg_t) b + sizeof(size_t) + b->size == (reg_t) p->next;
  } else {
    nalign = false;
  }
	
  if (palign && nalign) {
    /* b lines up with p and p->next, join them all. */
    p->size += sizeof(size_t) * 2 + b->size + p->next->size;
    p->next = p->next->next;
    b = p;
		
  } else if (nalign) {
    /* b lines up with p->next, join them. */
    b->size += sizeof(size_t) + p->next->size;
    b->next = p->next->next;
    p->next = b;
    
  } else if (palign) {
    /* b lines up with end of p, join them. */
    p->size += sizeof(size_t) + b->size;
    b = p;

  } else {
    /* b is somewhere between p and p->next. */
    b->next = p->next;
    p->next = b;
  }

 done:

  pp = nil;
  for (pl = pages; pl != nil; pp = pl, pl = pl->next) {
    if (pl->p->pa == (reg_t) b) {
      if (b->size + sizeof(size_t) == PAGE_SIZE) {
	printf("heap can free a page ! 0x%h\n", pl->p->pa);

	if (pp == nil) {
	  pages = pl->next;
	} else {
	  pp->next = pl->next;
	}

	pagefree(pl->p);
	free(pl);
	break;
      }
    }
  }

  unlock(&heaplock);
}
