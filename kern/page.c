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

static void
fixaddresses(struct pagel *p, struct pagel *next, reg_t addr)
{
  while (p->next != nil) {
    p->va = addr;
    addr += PAGE_SIZE;
    p = p->next;
  }

  p->va = addr;
  p->next = next;
}

void *
insertpages(struct mgroup *m, struct pagel *pn, size_t size)
{
  struct pagel *p;
  reg_t addr;

  lock(&m->lock);

  if (m->pages == nil) {
    fixaddresses(pn, nil, nil);
    m->pages = pn;
    unlock(&m->lock);
    return nil;
  }
  
  for (p = m->pages; p != nil && pn != nil; p = p->next) {
    if (p->next == nil ||
	p->va + PAGE_SIZE + size <= p->next->va) {

      addr = p->va + PAGE_SIZE;

      fixaddresses(pn, p->next, addr);
      p->next = pn;
      
      unlock(&m->lock);
      return (void *) addr;
    }
  }

  unlock(&m->lock);
  return nil;
}

void
insertpagesfixed(struct mgroup *m, struct pagel *pn, size_t size)
{
  struct pagel *p, *pp, *t;

  lock(&m->lock);

  if (m->pages == nil) {
    m->pages = pn;
    unlock(&m->lock);
    return;
  }

  pp = nil;
  p = m->pages;
  while (p != nil) {
    t = p->next;

    if (p->va == pn->va) {
      /* replace */

      if (pp == nil) {
	pp = pn->next;
	m->pages = pn;
	pn->next = p->next;
	pn = pp;
	pp = m->pages;
      } else {
	pp->next = pn;
	pn = pn->next;
	pp->next->next = p->next;
	pp = pp->next;
      }

      pagefree(p->p);
      free(p);
    }

    p = t;
  }

  if (pn != nil) {
    if (pp == nil) {
      m->pages = pn;
    } else {
      pp->next = pn;
    }
  }

  unlock(&m->lock);
}

struct pagel *
getrampages(size_t len, bool rw)
{
  struct pagel *head, *pl, *pp;
  struct page *pg;
  size_t l;

  head = pp = nil;
  for (l = 0; l < len; l += PAGE_SIZE) {
    pg = getrampage();
    if (pg == nil) {
      pagelfree(head);
      return nil;
    }

    pl = wrappage(pg, nil, rw, true);
    if (pl == nil) {
      pagefree(pg);
      pagelfree(head);
      return nil;
    }

    if (pp == nil) {
      head = pl;
    } else {
      pp->next = pl;
    }

    pp = pl;
  }

  return head;
}

struct pagel *
getiopages(void *addr, size_t len, bool rw)
{
  struct pagel *head, *pl, *pp;
  struct page *pg;
  size_t l;

  head = pp = nil;
  for (l = 0; l < len; l += PAGE_SIZE) {
    pg = getiopage(addr + l);
    if (pg == nil) {
      pagelfree(head);
      return nil;
    }

    pl = wrappage(pg, nil, rw, false);
    if (pl == nil) {
      pagefree(pg);
      pagelfree(head);
      return nil;
    }

    if (pp == nil) {
      head = pl;
    } else {
      pp->next = pl;
    }

    pp = pl;
  }

  return head;
}

