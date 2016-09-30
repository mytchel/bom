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

struct page *
newrampage(void)
{
  struct page *p;

  p = rampages.next;
  if (p == nil) {
    printf("No free pages!\n");
    return nil;
  }

  rampages.next = p->next;
  p->next = nil;
  p->refs = 1;

  return p;
}

void
freepage(struct page *p)
{
  atomicdec(&p->refs);

  if (p->refs > 0) {
    return;
  }

  lock(&p->lock);
  lock(&p->from->lock);

  p->next = p->from->next;
  p->from->next = p;

  unlock(&p->from->lock);
}

struct page *
copypages(struct page *pages, bool share)
{
  struct page *npages, *p, *pp, *op;

  pp = nil;
  npages = nil;
  for (op = pages; op != nil; op = op->next) {
    if (!share && op->type == PAGE_rw) {
      p = newrampage();
      if (p == nil) {
	goto err;
      }

      p->va = op->va;
      memmove(p->pa, op->pa, PAGE_SIZE);
    } else {
      p = op;
      atomicinc(&op->refs);
    }

    if (pp == nil) {
      npages = p;
    } else {
      pp->next = p;
    }

    pp = p;
  }

  return npages;

err:
  p = npages;
  while (p != nil) {
    pp = p->next;
    freepage(p);
    p = pp;
  }

  return nil;
}

static struct page *
findpageinlist(struct page *p, void *addr)
{
  printf("findpageinlist 0x%h\n", addr);
  while (p != nil) {
    printf("findpageinlist 0x%h in 0x%h to 0x%h\n", addr, p->va, (uint32_t) p->va + PAGE_SIZE);
    if (p->va <= addr && p->va + PAGE_SIZE > addr) {
      return p;
    }
    p = p->next;
  }
	
  return nil;
}

static struct page *
findpage(struct proc *p, void *addr)
{
  struct page *pg;

  pg = findpageinlist(current->pages, addr);
  if (pg == nil)
    pg = findpageinlist(current->stack, addr);
  return pg;
}

bool
fixfault(void *addr)
{
  struct page *pg;
	
  pg = findpage(current, addr);
  if (pg == nil) {
    return false;
  }
	
  mmuputpage(pg, pg->type != PAGE_ro, pg->type != PAGE_rws);
  current->faults++;
	
  return true;
}

void *
kaddr(struct proc *p, void *addr, size_t len)
{
  struct page *pg, *pn;
  uint32_t offset;

  printf("kaddr %i find 0x%h\n", p->pid, addr);
  pg = findpage(p, addr);
  if (pg == nil) {
    printf("kaddr couldnt find 0x%h for %i\n", addr, p->pid);
    return nil;
  }

  offset = (uint32_t) addr - (uint32_t) pg->va;

  if (offset + len >= PAGE_SIZE) {
    len -= PAGE_SIZE - offset;
    pn = pg;
    while (pn != nil && len >= PAGE_SIZE) {
      if (pn->next == nil) {
	printf("%i 0x%h to %i not in range\n", p->pid, addr, len);
	return nil;
      }
      
      if (pn->va + PAGE_SIZE != pn->next->va) {
	printf("%i 0x%h to %i not in range\n", p->pid, addr, len);
	return nil;
      } else {
	len -= PAGE_SIZE;
	pn = pn->next;
      }
    }
  }
  
  return pg->pa + offset;
}

struct page *
getpages(struct page *pages, void *pa, size_t *size)
{
  struct page *start, *p, *pp;
  size_t s;
	
  pp = pages;
  for (p = pp->next; p != nil; pp = p, p = p->next) {
    if (p->pa == pa) {
      break;
    }
  }
	
  if (p == nil)
    return nil;
	
  start = p;
  s = 0;
  while (p != nil) {
    s += PAGE_SIZE;
    if (s >= *size) {
      break;
    } else {
      p = p->next;
    }
  }

  if (p != nil) {
    pp->next = p->next;
    p->next = nil;
  } else {
    pp->next = nil;
  }
	
  *size = s;	
  return start;
}

