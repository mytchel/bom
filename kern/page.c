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

void
pagefree(struct page *p)
{
  if (atomicdec(&p->refs) > 0) {
    return;
  }

  p->next = *p->from;
  *(p->from) = p;
}

void
pagelfree(struct pagel *p)
{
  if (p == nil) {
    return;
  } else {
    pagefree(p->p);
    pagelfree(p->next);
    free(p);
  }
}

void
mgroupfree(struct mgroup *m)
{
  if (atomicdec(&m->refs) > 0) {
    return;
  }

  pagelfree(m->pages);
  free(m);
}

struct mgroup *
mgroupnew(void)
{
  struct mgroup *new;

  new = malloc(sizeof(struct mgroup));
  if (new == nil) {
    return nil;
  }

  new->refs = 1;
  new->pages = nil;
  memset(&new->lock, 0, sizeof(new->lock));

  return new;
}

struct mgroup *
mgroupcopy(struct mgroup *old)
{
  struct mgroup *new;
  new = mgroupnew();
  if (new == nil) {
    return nil;
  }
  
  lock(&old->lock);
  new->pages = pagelcopy(old->pages);
  unlock(&old->lock);
  
  return new;
}

struct pagel *
pagelcopy(struct pagel *po)
{
  struct pagel *new, *pn, *pp;
  struct page *pg;

  new = pp = nil;
  
  for (; po != nil; po = po->next) {
    if (po->p->forceshare || !po->rw) {
      pg = po->p;
      atomicinc(&pg->refs);
    } else {
      pg = getrampage();
      if (pg == nil) {
	goto err;
      }

      memmove((void *) pg->pa, (void *) po->p->pa, PAGE_SIZE);
    }

    pn = wrappage(pg, po->va, po->rw, po->c);
    if (pn == nil) {
      goto err;
    }

    if (pp == nil) {
      new = pn;
    } else {
      pp->next = pn;
    }

    pp = pn;
  }

  return new;

err:
  pagelfree(new);

  return nil;
}

struct pagel *
wrappage(struct page *p, reg_t va, bool rw, bool c)
{
  struct pagel *pl;

  pl = malloc(sizeof(struct pagel));
  if (pl == nil) {
    return nil;
  }

  pl->p = p;
  pl->va = va;
  pl->rw = rw;
  pl->c = c;
  pl->next = nil;

  return pl;
}

static struct pagel *
findpagelinlist(struct pagel *p, const reg_t addr)
{
  while (p != nil) {
    if (p->va <= addr && p->va + PAGE_SIZE > addr) {
      return p;
    } else {
      p = p->next;
    }
  }
	
  return nil;
}

static struct pagel *
findpagel(struct proc *p, const reg_t addr)
{
  struct pagel *pl = nil;

  if (p->mgroup) {
    pl = findpagelinlist(p->mgroup->pages, addr);
    if (pl != nil) return pl;
  }

  pl = findpagelinlist(p->ustack, addr);
  return pl;
}

bool
fixfault(void *vaddr)
{
  struct pagel *pl;
  struct page *pg;
  reg_t addr = (reg_t) vaddr;

  pl = findpagel(up, (reg_t) addr);
  if (pl != nil) {
    mmuputpage(pl);
    return true;
  } else if (up->ustack == nil) {
    return false;
  }

  /* Possibly extend stack */
  
  for (pl = up->ustack; pl->next != nil; pl = pl->next)
    ;

  if (addr < pl->va && addr > pl->va - PAGE_SIZE) {
    pg = getrampage();
    if (pg == nil) {
      return false;
    }

    pl->next = wrappage(pg, pl->va - PAGE_SIZE, true, true);
    if (pl->next == nil) {
      return false;
    } else {
      mmuputpage(pl->next);
      return true;
    }
    
  } else {
    return false;
  }
}

static bool
nilbyteinrange(struct pagel *pl, reg_t offset)
{
  struct pagel *pn;

  pn = pl;
  while (pn != nil) {
    while (offset < PAGE_SIZE) {
      if (*((uint8_t *) pn->p->pa + offset) == 0) {
	return true;
      } else {
	offset++;
      }
    }

    if (pn->next == nil) {
      return false;
    } else if (pn->va + PAGE_SIZE != pn->next->va) {
      return false;
    } else {
      offset = 0;
    }
  }

  return false;
}

static bool
leninrange(struct pagel *pl, reg_t offset, size_t len)
{
  struct pagel *pn;

  len -= PAGE_SIZE - offset;
  pn = pl;
  while (pn != nil && len >= PAGE_SIZE) {
    if (pn->next == nil) {
      return false;
    } else if (pn->va + PAGE_SIZE != pn->next->va) {
      return false;
    } else {
      len -= PAGE_SIZE;
      pn = pn->next;
    }
  }

  return true;
}

void *
kaddr(struct proc *p, const void *addr, size_t len)
{
  struct pagel *pl;
  uint32_t offset;

  pl = findpagel(p, (reg_t) addr);
  if (pl == nil) {
    return nil;
  }

  offset = (uint32_t) addr - (uint32_t) pl->va;

  if (len == 0) {
    if (!nilbyteinrange(pl, offset)) {
      return nil;
    }
  } else if (offset + len >= PAGE_SIZE) {
    if (!leninrange(pl, offset, len)) {
      return nil;
    }
  }
  
  return (void *) (pl->p->pa + offset);
}

static reg_t
insertpagesfix(struct mgroup *m, struct pagel *pn,
	       reg_t addr, size_t size)
{
  struct pagel *p, *pp;
  reg_t caddr;

  caddr = (reg_t) addr;
  pp = nil;
  for (p = m->pages; p != nil && pn != nil; pp = p, p = p->next) {
    if (p->next == nil ||
	p->va + PAGE_SIZE + size <= p->next->va) {

      caddr = (reg_t) p->va + PAGE_SIZE;
      addr = caddr;
      
      for (pp = pn;  pp != nil && pp->next != nil; pp = pp->next) {
	pp->va = caddr;
	caddr += PAGE_SIZE;
      }

      pp->va = caddr;

      pp->next = p->next;
      p->next = pn;

      return addr;
    }
  }

  return 0;
}

static reg_t
insertpagesnofix(struct mgroup *m, struct pagel *pn,
		 reg_t addr, size_t size)
{
  struct pagel *p, *pp;
  reg_t caddr;

  caddr = (reg_t) addr;
  pp = nil;
  for (p = m->pages; p != nil && pn != nil; pp = p, p = p->next) {
    if ((reg_t) p->va == caddr) {
      /* Replace page */

      printf("replace page at 0x%h\n", p->va);

      if (pp == nil) {
	pp = m->pages->next;
	m->pages = pn;
	
	pn = pn->next;
	m->pages->next = pp;
      } else {
	pp->next = pn;
	pn = pn->next;
	pp->next->next = p->next;
      }

      caddr += PAGE_SIZE;

      pagefree(p->p);
      free(p);
    }
  }

  return addr;
}

reg_t
insertpages(struct mgroup *m, struct pagel *pn,
	    reg_t addr, size_t size, bool fix)
{
  reg_t caddr;

  if (m->pages == nil) {
    m->pages = pn;

    if (fix) {
      addr = nil;
      caddr = 0;
      while (fix && pn != nil) {
	pn->va = caddr;
	pn = pn->next;
	caddr += PAGE_SIZE;
      }

      return nil;
    } else {
      return addr;
    }
  
  } else {
    if (fix) {
      return insertpagesfix(m, pn, addr, size);
    } else {
      return insertpagesnofix(m, pn, addr, size);
    }
  }
}

    #if 0

static void
fixpagel(struct pagel *p, reg_t addr, struct pagel *next)
{
  struct pagel *pp;

  pp = nil;
  for (pp = nil; p != nil; pp = p, p = p->next) {
    p->va = (void *) addr;
    addr += PAGE_SIZE;
  }
	
  if (pp != nil) {/* Should never be nil. */
    pp->next = next;
  }
}

if (fix && pp != nil) {
      addr = (uint8_t *) pp->va + PAGE_SIZE;
    }
		
    if ((size_t) (p->va - addr) > size) {
      /* Pages fit in here */
      break;
    }
  }

  if (pp == nil) {
    up->mgroup->pages = pagel;
    fixpagel(pagel, (reg_t) addr, p);
  } else {
    if (fix) {
      addr = (uint8_t *) pp->va + PAGE_SIZE;
    }
    
    pp->next = pagel;
    fixpagel(pagel, (reg_t) addr, p);
  }
  
  return addr;
}

#endif
