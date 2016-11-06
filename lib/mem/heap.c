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

#include <libc.h>
#include <mem.h>

struct block {
  size_t size;
  struct block *next;
};

static struct block *heap = nil;

struct block *
growheap(struct block *prev, size_t size)
{
  struct block *b;
  void *pg;

  pg = mmap(MEM_ram, nil, &size);
  if (pg == nil) {
    return nil;
  }

  if (prev == nil) {
    b = heap = (struct block *) pg;
  } else {
    b = prev->next = (struct block *) pg;
  }
	
  b->size = size - sizeof(size_t);
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

  p = nil;
  for (b = heap; b != nil; p = b, b = b->next) {
    if (b->size >= size) {
      break;
    }
  }

  if (b == nil) {
    b = growheap(p, size);
    if (b == nil) {
      return nil;
    }
  }

  ptr = (void *) ((uint8_t *) b + sizeof(size_t));
	
  if (b->size > size + sizeof(size_t) + sizeof(struct block)) {
    n = (struct block *) ((uint8_t *) ptr + size);
		
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

  return ptr;
}

void
free(void *ptr)
{
  struct block *b, *p;
  bool palign, nalign;

  if (ptr == nil)
    return;

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

  for (p = heap; p != nil && p->next != nil; p = p->next) {
    if ((void *) p->next > ptr && ptr < (void *) p->next->next) {
      break;
    }
  }
	
  if (p == nil) {
    printf("Are you sure 0x%h is from the heap?\n", ptr);
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
}
