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

reg_t
sysexit(va_list args)
{
  int code = va_arg(args, int);

  printf("pid %i exited with status %i\n", 
	 current->pid, code);

  disableintr();
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
  if (ms < 0) {
    ms = 0;
  }

  disableintr();

  if (ms == 0) {
    procyield(current);
  } else {
    procsleep(current, ms);
  }

  schedule();

  enableintr();
		
  return 0;
}

reg_t
sysfork(va_list args)
{
  struct proc *p;
  int flags;
	
  flags = va_arg(args, int);

  p = newproc(current->priority);
  if (p == nil) {
    return ENOMEM;
  }

  p->inkernel = false;
  p->parent = current;

  p->dot = copypath(current->dot);
  p->dotchan = current->dotchan;
  atomicinc(&p->dotchan->refs);

  p->ustack = copypagel(current->ustack);

  if (flags & FORK_smem) {
    p->mgroup = current->mgroup;
    atomicinc(&current->mgroup->refs);
  } else {
    p->mgroup = copymgroup(current->mgroup);
  }

  if (flags & FORK_sfgroup) {
    p->fgroup = current->fgroup;
    atomicinc(&p->fgroup->refs);
  } else {
    p->fgroup = copyfgroup(current->fgroup);
    if (p->fgroup == nil) {
      goto err;
    }
  }
	
  if (flags & FORK_sngroup) {
    p->ngroup = current->ngroup;
    atomicinc(&p->ngroup->refs);
  } else {
    p->ngroup = copyngroup(current->ngroup);
    if (p->ngroup == nil) {
      goto err;
    }
  }

  forkchild(p, current->ureg);
  procready(p);

  return p->pid;

 err:
  procremove(p);
  return ENOMEM;
}

int
getpid(void)
{
  return current->pid;
}

reg_t
sysgetpid(va_list args)
{
  return current->pid;
}

reg_t
syswaitintr(va_list args)
{
  int irqn;

  irqn = va_arg(args, int);

  if (procwaitintr(current, irqn)) {
    return OK;
  } else {
    return ERR;
  }
}

static void
fixpagel(struct pagel *p, reg_t offset, struct pagel *next)
{
  struct pagel *pp;

  pp = nil;
  while (p != nil) {
    p->va = (uint8_t *) p->va + offset;
    pp = p;
    p = p->next;
  }
	
  if (pp != nil) /* Should never be nil. */
    pp->next = next;
}

static void *
insertpages(struct pagel *pagel, void *addr, size_t size)
{
  struct pagel *p, *pp;
  bool fix;

  fix = (addr == nil);

  pp = nil;
  for (p = current->mgroup->pages; p != nil; pp = p, p = p->next) {
    if (fix && pp != nil) {
      addr = (uint8_t *) pp->va + PAGE_SIZE;
    }
		
    if ((size_t) (p->va - addr) > size) {
      /* Pages fit in here */
      break;
    }
  }
	
  if (pp != nil) {
    if (fix) {
      addr = (uint8_t *) pp->va + PAGE_SIZE;
    }
    
    fixpagel(pagel, (reg_t) addr, p);
    pp->next = pagel;
  } else {
    /* First page */
    fixpagel(pagel, (reg_t) addr, p);
    current->mgroup->pages = pagel;
  }

  return addr;
}

reg_t
sysgetmem(va_list args)
{
  struct pagel *pagel, *pp, *pl;
  struct page *pg;
  size_t *size, csize;
  void *addr, *caddr, *paddr;
  bool rw, c;
  int type;
	
  type = va_arg(args, int);
  addr = va_arg(args, void *);
  size = va_arg(args, size_t *);

  switch (type) {
  case MEM_ram:
    rw = true;
    c = true;
    paddr = nil;
    caddr = addr;
    break;
  case MEM_io:
    rw = true;
    c = false;
    paddr = addr;
    addr = nil;
    caddr = nil;
    break;
  default:
    return ERR;
  }

  pg = nil;
  pp = pagel = nil;
  csize = 0;

  while (csize < *size) {
    switch (type) {
    case MEM_ram:
      pg = getrampage();
      break;
    case MEM_io:
      pg = getiopage(paddr);
      paddr += PAGE_SIZE;
      break;
    }

    if (pg == nil) {
      freepage(pg);
      freepagel(pagel);
      return ERR;
    }
    
    pl = wrappage(pg, caddr, rw, c);
    if (pl == nil) {
      freepagel(pagel);
      return ERR;
    }

    if (pp == nil) {
      pagel = pl;
    } else {
      pp->next = pl;
    }

    caddr += PAGE_SIZE;
    csize += PAGE_SIZE;
  }
	
  if (pagel == nil) {
    return nil;
  }

  *size = csize;

  lock(&current->mgroup->lock);
  addr = insertpages(pagel, addr, csize);
  unlock(&current->mgroup->lock);

  return (reg_t) addr;
}

reg_t
sysrmmem(va_list args)
{
  struct pagel *p, *pt, *pp;
  void *addr;
  size_t size;

  addr = va_arg(args, void *);
  size = va_arg(args, size_t);
	
  lock(&current->mgroup->lock);
  
  pp = nil;
  p = current->mgroup->pages; 
  while (p != nil && size > 0) {
    if (p->va == addr) {
      addr += PAGE_SIZE;
      size -= PAGE_SIZE;
			
      if (pp != nil) {
	pp->next = p->next;
      } else {
	current->mgroup->pages = p->next;
      }
			
      pt = p->next;

      freepage(p->p);
      free(p);

      p = pt;
    } else {
      pp = p;
      p = p->next;
    }
  }
	
  unlock(&current->mgroup->lock);

  return OK;
}

