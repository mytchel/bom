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

struct segment *
newseg(int type)
{
  struct segment *s;

  s = malloc(sizeof(struct segment));
  if (s == nil) {
    return nil;
  }

  s->refs = 1;	
  s->type = type;
  s->pages = nil;

  return s;
}

void
freeseg(struct segment *s)
{
  struct page *p, *pp;
	
  if (atomicdec(&s->refs) > 0)
    return;
	
  p = s->pages;
  while (p != nil) {
    pp = p;
    p = p->next;
    freepage(pp);
  }
	
  free(s);
}

struct segment *
copyseg(struct segment *s, bool copy)
{
  struct segment *n;
  struct page *sp, *np;

  if (!copy || s->type == SEG_ro) {
    /* No need to actually copy. */
    atomicinc(&s->refs);
    n = s;
  } else {
    /* Get new pages and copy the data from old. */
    n = newseg(s->type);
    if (n == nil)
      return nil;

    sp = s->pages;
    if (sp == nil)
      return n;

    n->pages = newpage(sp->va);
    np = n->pages;
    while (sp != nil) {
      memmove(np->pa, sp->pa, sp->size);
      if (sp->next == nil)
	break;
			
      sp = sp->next;
      np->next = newpage(sp->va);
      np = np->next;
    }
  }
	
  return n;
}

static struct page *
findpage(struct proc *p, void *addr, struct segment **seg)
{
  struct segment *s;
  struct page *pg;
  int i;

  for (i = 0; i < Smax; i++) {
    s = p->segs[i];
    for (pg = s->pages; pg != nil; pg = pg->next) {
      if (pg->va <= addr && pg->va + pg->size > addr) {
	*seg = s;
	return pg;
      }
    }
  }
	
  return nil;
}

bool
fixfault(void *addr)
{
  struct segment *s;
  struct page *pg;
	
  pg = findpage(current, addr, &s);
  if (pg == nil) return false;
	
  mmuputpage(pg, s->type == SEG_rw, pg->from != &iopages);
  current->faults++;
	
  return true;
}

void *
kaddr(struct proc *p, void *addr, size_t len)
{
  struct segment *s;
  struct page *pg, *pn;
  uint32_t offset;
	
  pg = findpage(p, addr, &s);
  if (pg == nil) return nil;

  offset = (uint32_t) addr - (uint32_t) pg->va;

  if (offset + len >= pg->size) {
    len -= pg->size - offset;
    pn = pg;
    while (pn != nil) {
      if (pn->next == nil) {
	return nil;
      }
      
      if (pn->va + pn->size != pn->next->va) {
	return nil;
      } else {
	len -= pn->size;
	pn = pn->next;
	if (len < pn->size) {
	  break;
	}
      }
    }
  }
  
  return pg->pa + offset;
}

struct page *
newpage(void *va)
{
  struct page *p;

  p = rampages.next;
  if (p == nil) {
    printf("No free pages!\n");
    return nil;
  }

  rampages.next = p->next;
  p->next = nil;
  p->va = va;

  return p;
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
    s += p->size;
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

void
freepage(struct page *p)
{
  p->next = p->from->next;
  p->from->next = p;
}

