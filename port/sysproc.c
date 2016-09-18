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

reg_t
sysexit(va_list args)
{
  int code = va_arg(args, int);

  debug("pid %i exited with status %i\n", 
	 current->pid, code);

  procremove(current);
  schedule();
	
  /* Never reached. */
  return code; /*ERR;*/
}

reg_t
syssleep(va_list args)
{
  int ms;

  ms = va_arg(args, int);
  if (ms <= 0) {
    schedule();
  } else {
    procsleep(current, ms);
  }
		
  return 0;
}

reg_t
sysfork(va_list args)
{
  int i;
  struct proc *p;
  int flags;
  bool copy;
	
  flags = va_arg(args, int);

  p = newproc();
  if (p == nil)
    return ERR;

  p->parent = current;
  p->quanta = current->quanta;
  p->dot = copypath(current->dot);

  copy = !(flags & FORK_smem);
  for (i = 0; i < Smax; i++) {
    p->segs[i] = copyseg(current->segs[i], copy || (i == Sstack));
    if (p->segs[i] == nil) {
      /* Should do better cleaning up here. */
      return ENOMEM;
    }
  }
	
  if (flags & FORK_sfgroup) {
    p->fgroup = current->fgroup;
    p->fgroup->refs++;
  } else {
    p->fgroup = copyfgroup(current->fgroup);
  }
	
  if (flags & FORK_sngroup) {
    p->ngroup = current->ngroup;
    p->ngroup->refs++;
  } else {
    p->ngroup = copyngroup(current->ngroup);
  }
	
  forkchild(p, current->ureg);
  procready(p);

  return p->pid;
}

reg_t
sysgetpid(va_list args)
{
  return current->pid;
}

static void
fixpages(struct page *pg, reg_t offset, struct page *next)
{
  struct page *p, *pp;

  pp = nil;
  for (p = pg; p != nil; pp = p, p = p->next)
    p->va = (uint8_t *) p->va + offset;
	
  if (pp != nil) /* Should never be nil. */
    pp->next = next;
}

static struct page *
getnewpages(size_t *size)
{
  struct page *pages, *p, *pt;
  size_t s;
	
  pages = newpage(0);
  if (pages == nil) {
    return nil;
  }
	
  p = pages;
  s = pages->size;
  while (s < *size) {
    p->next = newpage(p->va + p->size);
    if (p->next == nil) {
      p = pages;
      while (p != nil) {
	pt = p->next;
	freepage(p);
	p = pt;
      }
      return nil;
    }
		
    p = p->next;
    s += p->size;
  }
	
  *size = s;
  return pages;	
}

static void *
startofheap(void)
{
  struct page *p;
  void *addr = nil;

  for (p = current->segs[Stext]->pages;
       p != nil;
       p = p->next)
    if ((uint8_t *) p->va + p->size > (uint8_t *) addr)
      addr = (uint8_t *) p->va + p->size;

  for (p = current->segs[Sdata]->pages;
       p != nil;
       p = p->next)
    if ((uint8_t *) p->va + p->size > (uint8_t *) addr)
      addr = (uint8_t *) p->va + p->size;

  return addr;
}

static void *
insertpages(struct page *pages, void *addr, size_t size)
{
  struct page *p, *pp;
  bool fix;

  fix = (addr == nil);

  if (fix)
    addr = startofheap();

  pp = nil;
  for (p = current->segs[Sheap]->pages;
       p != nil; pp = p, p = p->next) {
		
    if (fix && pp != nil) 
      addr = (uint8_t *) pp->va + pp->size;
		
    if ((size_t) (p->va - addr) > size) {
      /* Pages fit in here */
      break;
    }
  }
	
  if (pp != nil) {
    if (fix) addr = (uint8_t *) pp->va + pp->size;
    fixpages(pages, (reg_t) addr, p);
    pp->next = pages;
  } else {
    /* First page on heap. */
    fixpages(pages, (reg_t) addr, p);
    current->segs[Sheap]->pages = pages;
  }

  return addr;
}

static bool
addrsinuse(void *start, size_t size)
{
  struct page *p;
  int i;
  void *end = (uint8_t *) start + size;
	
  for (i = 0; i < Smax; i++) {
    for (p = current->segs[i]->pages; p != nil; p = p->next) {
      if (p->va == start) {
	return true;
      } else if (p->va < start 
		 && start < (void *) ((uint8_t *) p->va + p->size)) {
	/* Start of block in page */
	return true;
      } else if (p->va < end 
		 && end < (void *) ((uint8_t *) p->va + p->size)) {
	/* End of block in page */
	return true;
      } else if (start <= p->va && p->va <= end) {
	/* Block encompases page */
	return true;
      }
    }
  }
	
  return false;
}

reg_t
sysgetmem(va_list args)
{
  struct page *pages = nil;
  void *addr;
  size_t *size;
  int type;
	
  type = va_arg(args, int);
  addr = va_arg(args, void *);
  size = va_arg(args, size_t *);

  debug("getmem 0x%h, %i\n", addr, *size);
  
  if (*size > MAX_MEM_SIZE) {
    return nil;
  }
	
  if (addr != nil && addrsinuse(addr, *size)) {
    return nil;
  }

  switch (type) {
  case MEM_heap:
    pages = getnewpages(size);
    break;
  case MEM_io:
    pages = getpages(&iopages, addr, size);
    addr = nil;
    break;
  }
	
  if (pages == nil) {
    return nil;
  }

  addr = insertpages(pages, addr, *size);

  debug("getmem success, put %i bytes at 0x%h\n", *size, addr);
  return (reg_t) addr;
}

reg_t
sysrmmem(va_list args)
{
  struct page *p, *pt, *pp;
  void *addr;
  size_t size;

  addr = va_arg(args, void *);
  size = va_arg(args, size_t);
	
  debug("Should unmap 0x%h of len %i from %i\n", addr, size, current->pid);
	
  pp = nil;
  p = current->segs[Sheap]->pages; 
  while (p != nil && size > 0) {
    if (p->va == addr) {
      addr = (uint8_t *) p->va + p->size;
      size -= p->size;
			
      if (pp != nil) {
	pp->next = p->next;
      } else {
	current->segs[Sheap]->pages = p->next;
      }
			
      pt = p->next;
      freepage(p);
      p = pt;
    } else {
      pp = p;
      p = p->next;
    }
  }
	
  return ENOIMPL;
}

reg_t
syswaitintr(va_list args)
{
  int irqn;

  irqn = va_arg(args, int);

  disableintr();

  if (procwaitintr(current, irqn)) {
    debug("%i waiting for irq %i\n", current->pid, irqn);
    procwait(current);
    /* schedule enables interrupts */
    schedule();
    return OK;
  } else {
    debug("%i wait for irq %i failed\n", current->pid, irqn);
    enableintr();
    return ERR;
  }
}
