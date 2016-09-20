/*
 *   Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <libc.h>

struct block {
  size_t size;
  struct block *next;
};

static struct block *heap = nil;

struct block *
growheap(struct block *prev, size_t size)
{
  struct block *b;


  size += sizeof(size_t);

  b = getmem(MEM_heap, nil, &size);
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

static void *
roundtoptr(void *p)
{
  reg_t x, r;
  x = (reg_t) p;
  r = x % sizeof(void *);
  if (r == 0) {
    return p;
  } else {
    return (void *) (x + sizeof(void *) - r);
  }
}

void *
malloc(size_t size)
{
  struct block *b, *n, *p;
  void *block;

  if (size == 0)
    return nil;

  size = (size_t) roundtoptr((void *) size);

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
    printf("%i, Are you sure 0x%h is from the heap?\n",
	   getpid(), ptr);
    return;
  } else if (p->next == nil) {
    /* b is at end of list, append. */
    b->next = nil;
    p->next = b;
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
}
