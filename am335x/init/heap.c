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

#define CHUNK_POWER_MIN 2

struct fixedblk {
  size_t num, numused;
  uint8_t *used;
  void *start;
  struct fixedblk *next;
};

static struct fixedblk *
heaps[1 + CHUNK_POWER_MAX - CHUNK_POWER_MIN] = {nil};

static void
addchunk(int power, void *start, size_t size)
{
  struct fixedblk *new = start;
  int i, csize;

  if (power < CHUNK_POWER_MIN) {
    return;
  }

  new->num  = 0;
  csize = sizeof(struct fixedblk);
  while (csize + roundptr(new->num * sizeof(uint8_t))
	 + roundptr(new->num * (1 << power)) < size) {
    new->num++;
  }

  if (new->num == 0) {
    return;
  } else {
    new->num--;
  }

  if (new->num < 2) {
    return addchunk(power - 1, start, size);
  }

  csize += roundptr(new->num * sizeof(uint8_t))
    + roundptr(new->num * (1 << power));

  if (size - csize > sizeof(struct fixedblk)
                      + (2 << (power - 1))) {

    addchunk(power - 1, start + csize, size - csize);
  }

  new->used = (void *) ((reg_t) start + sizeof(struct fixedblk));
  new->start = (void *) roundptr(((reg_t) new->used +
				  sizeof(uint8_t) * new->num));

  new->numused = 0;
  for (i = 0; i < new->num; i++) {
    new->used[i] = 0;
  }

  new->next = heaps[power - CHUNK_POWER_MIN];
  heaps[power - CHUNK_POWER_MIN] = new;
}

static void
growchunk(int power)
{
  void *blk;
  size_t size;

  size = (1 << power) * 100;

  blk = getmem(MEM_heap, nil, &size);

  addchunk(power, blk, size);
}

static void *
getchunk(int power)
{
  struct fixedblk *blk;
  int i;

  blk = heaps[power-CHUNK_POWER_MIN];

  for (; blk != nil; blk = blk->next) {
    if (blk->numused == blk->num) {
      continue;
    }
    
    for (i = 0; i < blk->num; i++) {
      if (!blk->used[i]) {
	blk->numused++;
	blk->used[i] = true;
	return blk->start + (1 << power) * i;
      }
    }
  }

  printf("Not chunk for %i found\n", 1 << power);
  growchunk(power);
  return getchunk(power);
}

void *
malloc(size_t size)
{
  void *ptr;
  int p;

  if (size == 0)
    return nil;

  ptr = nil;
  for (p = CHUNK_POWER_MIN; p <= CHUNK_POWER_MAX; p++) {
    if (size < (1 << p)) {
      ptr = getchunk(p);
      break;
    }
  }

  return ptr;
}

void
free(void *ptr)
{
  struct fixedblk *blk;
  reg_t offset;
  int p;

  for (p = CHUNK_POWER_MIN; p <= CHUNK_POWER_MAX; p++) {
    blk = heaps[p-CHUNK_POWER_MIN];
    for (; blk != nil; blk = blk->next) {
      if ((reg_t) blk->start <= (reg_t) ptr &&
	  (reg_t) blk->start + ((1 << p) * blk->num) > (reg_t) ptr) {

	offset = (reg_t) ptr - (reg_t) blk->start;
	offset /= (1 << p);

	blk->used[offset] = false;
	blk->numused--;
	break;
      }
    }
  }
}
