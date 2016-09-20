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

  if (s->type == SEG_ro || !copy) {
    /* No need to actually copy. */
    atomicinc(&s->refs);
    n = s;
  } else {
    /* Gets new pages and copies the data from old. */
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
	
  mmuputpage(pg, s->type == SEG_rw);
  current->faults++;
	
  return true;
}

void *
kaddr(struct proc *p, void *addr)
{
  struct segment *s;
  struct page *pg;
	
  pg = findpage(p, addr, &s);
  if (pg == nil) return nil;

  return pg->pa + (addr - pg->va);
}

struct page *
newpage(void *va)
{
  struct page *p;

  p = pages.next;
  if (p == nil) {
    printf("No free pages!\n");
    return nil;
  }

  pages.next = p->next;
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

