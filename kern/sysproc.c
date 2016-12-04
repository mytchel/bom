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
sysexit(int code)
{
  setintr(INTR_off);
  procexit(up, code);
  schedule();
	
  /* Never reached. */
  return code; /*ERR;*/
}

reg_t
syssleep(int ms)
{
  if (ms < 0) {
    ms = 0;
  }

  if (ms == 0) {
    procyield();
  } else {
    procsleep(ms);
  }

  return 0;
}

reg_t
sysfork(int flags, struct label *ureg)
{
  struct proc *p;
  reg_t r;

  p = procnew();
  if (p == nil) {
    return ENOMEM;
  }

  p->dot = pathcopy(up->dot);
 
  p->dotchan = up->dotchan;
  if (p->dotchan != nil) {
    atomicinc(&p->dotchan->refs);
  }

  p->root = up->root;
  if (p->root != nil) {
    atomicinc(&p->root->refs);
  }
  
  p->ustack = pagelcopy(up->ustack);

  if (flags & FORK_smem) {
    p->mgroup = up->mgroup;
    atomicinc(&up->mgroup->refs);
  } else {
    p->mgroup = mgroupcopy(up->mgroup);
  }

  if (flags & FORK_sfgroup) {
    p->fgroup = up->fgroup;
    atomicinc(&p->fgroup->refs);
  } else {
    p->fgroup = fgroupcopy(up->fgroup);
    if (p->fgroup == nil) {
      goto err;
    }
  }
	
  if (flags & FORK_sngroup) {
    p->ngroup = up->ngroup;
    atomicinc(&p->ngroup->refs);
  } else {
    p->ngroup = ngroupcopy(up->ngroup);
    if (p->ngroup == nil) {
      goto err;
    }
  }

  p->parent = up;

  do {
    p->cnext = up->children;
  } while (!cas(&up->children, p->cnext, p));

  /* Both parent and child return from this */
  r = forkchild(p);

  return r;
  
 err:
  procexit(p, 0);
  return ENOMEM;
}

reg_t
sysexec(const char *upath, int argc, char *argv[])
{
  struct path *path;
  struct chan *c;
  int r, i;

  if (kaddr(up, upath, 0) == nil) {
    return ERR;
  }

  if (kaddr(up, argv, sizeof(char *) * argc) == nil) {
    return ERR;
  }

  for (i = 0; i < argc; i++) {
    if (kaddr(up, argv[i], 0) == nil) {
      return ERR;
    }
  }

  path = strtopath(up->dot, upath);

  c = fileopen(path, O_RDONLY, 0, &r);
  if (r != OK) {
    return r;
  }

  r = kexec(c, argc, argv);

  chanfree(c);
  
  return r;
}

int
getpid(void)
{
  return up->pid;
}

reg_t
sysgetpid(void)
{
  return up->pid;
}

reg_t
syswait(int *ustatus)
{
  struct proc *p;
  int *status, pid;

  status = kaddr(up, ustatus, sizeof(int *));
  if (status == nil) {
    return ERR;
  }
  
  p = waitchild();
  if (p == nil) {
    return ERR;
  } 

  *status = p->exitcode;
  pid = p->pid;
  
  procfree(p);

  return pid;
}

reg_t
syswaitintr(int irqn)
{
  if (procwaitintr(irqn)) {
    return OK;
  } else {
    return ERR;
  }
}

reg_t
sysmmap(int flags, size_t len, int fd, va_list ap)
{
  struct pagel *pages;
  struct chan *c;
  size_t offset;
  void *addr;
  bool rw;

  offset = va_arg(ap, size_t);
  addr = va_arg(ap, void *);

  len = PAGE_ALIGN(len);
  
  if (flags & MEM_rw) {
    rw = true;
  } else {
    rw = false;
  }

  if (flags & MEM_ram) {
    pages = getrampages(len, rw);

  } else if (flags & MEM_io) {
    pages = getiopages(addr, len, rw);

  } else if (flags & MEM_file) {
    c = fdtochan(up->fgroup, fd);
    if (c == nil) {
      return nil;
    }

    offset = PAGE_ALIGN(offset + PAGE_SIZE - 1);
    pages = getfilepages(c, offset, len, rw);
  } else {
    pages = nil;
  }

  if (pages != nil) {
    return (reg_t) insertpages(up->mgroup, pages, len);
  } else {
    return nil;
  }
}

reg_t
sysmunmap(void *addr, size_t size)
{
  struct pagel *p, *pt, *pp;
  reg_t ret;

  lock(&up->mgroup->lock);

  ret = OK;
  pp = nil;
  p = up->mgroup->pages; 

  while (p != nil && size > 0) {
    if (p->va == (reg_t) addr) {
      addr += PAGE_SIZE;
      size -= PAGE_SIZE;
			
      if (pp == nil) {
	up->mgroup->pages = p->next;
      } else {
	pp->next = p->next;
      }
			
      pt = p->next;

      pagefree(p->p);
      free(p);

      p = pt;
    } else {
      ret = ERR;
      break;
    }
  }
	
  unlock(&up->mgroup->lock);

  return ret;
}

