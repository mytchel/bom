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

static void addtolistfront(struct proc **, struct proc *);
static void addtolistback(struct proc **, struct proc *);
static bool removefromlist(struct proc **, struct proc *);

static uint32_t nextpid = 1;

static struct proc *ready = nil;
static struct proc *sleeping = nil;
static struct proc *suspended = nil;

static struct proc *nullproc;

struct proc *up = nil;

void
schedulerinit(void)
{
  nullproc = procnew();
  if (nullproc == nil) {
    panic("Failed to create null proc!\n");
  }

  nullproc->quanta = mstoticks(QUANTA_MIN);
  
  forkfunc(nullproc, &nullprocfunc, nil);
  procready(nullproc);
}

static struct proc *
nextproc(void)
{
  struct proc *p;

  p = ready;
  if (p != nil) {
    ready = p->snext;
  } else {
    p = nullproc;
  }

  return p;
}

static void
updatesleeping(uint32_t t)
{
  struct proc *p, *pp, *pt;

  pp = nil;
  p = sleeping;
  while (p != nil) {
    if (t >= p->sleep) {
      if (pp == nil) {
	sleeping = p->snext;
      } else {
	pp->snext = p->snext;
      }

      pt = p->snext;

      p->state = PROC_ready;
      addtolistback(&ready, p);

      p = pt;
    } else {
      p->sleep -= t;

      pp = p;
      p = p->snext;
    }
  }
}

void
schedule(void)
{
  uint32_t t;

  if (up != nil) {
    if (up->state == PROC_oncpu) {
      up->state = PROC_ready;

      if (up != nullproc) {
	addtolistback(&ready, up);
      }
    }

    if (setlabel(&up->label)) {
      return;
    }
  }

  t = ticks();

  updatesleeping(t);
	
  up = nextproc();
  up->state = PROC_oncpu;

  mmuswitch(up->mmu);

  setsystick(up->quanta);
  
  cticks();
  gotolabel(&up->label);
}
	
struct proc *
procnew(void)
{
  struct proc *p;
	
  p = malloc(sizeof(struct proc));
  if (p == nil) {
    return nil;
  }

  p->quanta = mstoticks(QUANTA_DEF);
  
  p->pid = nextpid++;

  p->mmu = mmunew();

  p->kstack = getrampage();
  if (p->kstack == nil) {
    free(p);
    return nil;
  }
 
  p->state = PROC_suspend;
  addtolistfront(&suspended, p);

  return p;
}

void
procexit(struct proc *p, int code)
{
  struct proc *par, *c;
  
  if (p->dotchan != nil) {
    chanfree(p->dotchan);
    p->dotchan = nil;
  }

  if (p->fgroup != nil) {
    fgroupfree(p->fgroup);
    p->fgroup = nil;
  }

  if (p->ngroup != nil) {
    ngroupfree(p->ngroup);
    p->ngroup = nil;
  }

  if (p->dot != nil) {
    pathfree(p->dot);
    p->dot = nil;
  }

  if (p->ustack != nil) {
    pagelfree(p->ustack);
    p->ustack = nil;
  }
 
  if (p->mgroup != nil) {
    mgroupfree(p->mgroup);
    p->mgroup = nil;
  }

  par = p->parent;

  for (c = p->children; c != nil; c = c->cnext) {
    c->parent = par;
  }

  if (par != nil) {
    while (true) {
      c = par->children;
      while (c != nil && c->cnext != nil)
	c = c->cnext;

      if (c == nil) {
	if (cas(&par->children, nil, p->children)) {
	  break;
	}
      } else if (cas(&c->cnext, nil, p->children)) {
	break;
      }
    }

    p->children = nil;

    while (true) {
      c = par->children;
      if (c == p) {
	if (cas(&par->children, p, p->cnext)) {
	  break;
	} else {
	  continue;
	}
      }

      while (c->cnext != p)
	c = c->cnext;

      if (cas(&c->cnext, p, p->cnext)) {
	break;
      }
    }

    do {
      p->cnext = par->deadchildren;
    } while (!cas(&par->deadchildren, p->cnext, p));

    if (par->state == PROC_waitchild) {
      procready(par);
    }

    p->exitcode = code;
    p->state = PROC_dead;
 
  } else {
    procfree(p);
  }

  if (p == up) {
    up = nil;
  }
}

void
procfree(struct proc *p)
{
  mmufree(p->mmu);
  pagefree(p->kstack);
  free(p);
}

struct proc *
waitchild(void)
{
  struct proc *p;
  intrstate_t i;

  if (up->deadchildren == nil) {
    if (up->children == nil) {
      return nil;
    } else {
      i = setintr(INTR_off);

      up->state = PROC_waitchild;
      schedule();

      setintr(i);
    }
  }

  do {
    p = up->deadchildren;
  } while (!(cas(&up->deadchildren, p, p->cnext)));
  
  return p;
}

void
procready(struct proc *p)
{
  if (p->state == PROC_suspend) {
    removefromlist(&suspended, p);
  }

  p->state = PROC_ready;
  addtolistfront(&ready, p);
}

void
procsleep(uint32_t ms)
{
  intrstate_t i;

  up->sleep = mstoticks(ms);
  addtolistfront(&sleeping, up);

  i = setintr(INTR_off);
  up->state = PROC_sleeping;
  
  schedule();
  setintr(i);
}

void
procyield(void)
{
  intrstate_t i;

  addtolistback(&ready, up);

  up->state = PROC_ready;
  i = setintr(INTR_off);
  schedule();
  setintr(i);
}

void
procsuspend(struct proc *p)
{
  if (p->state == PROC_ready) {
    removefromlist(&ready, p);
  }
  
  p->state = PROC_suspend;
  addtolistfront(&suspended, p);
}

void
procwait(void)
{
  intrstate_t i;

  i = setintr(INTR_off);

  if (up->state == PROC_oncpu) {
    up->state = PROC_waiting;
    schedule();
  }

  setintr(i);
}

void
addtolistfront(struct proc **l, struct proc *p)
{
  do {
    p->snext = *l;
  } while (!cas(l, p->snext, p));
}

void
addtolistback(struct proc **l, struct proc *p)
{
  struct proc *pp;

  p->snext = nil;
  
  while (true) {
    for (pp = *l; pp != nil && pp->snext != nil; pp = pp->snext)
      ;

    if (pp == nil) {
      if (cas(l, nil, p)) {
	break;
      }
    } else if (cas(&pp->snext, nil, p)) {
      break;
    }
  }
}

bool
removefromlist(struct proc **l, struct proc *p)
{
  struct proc *pt;

  while (true) {
    if (*l == p) {
      if (cas(l, p, p->snext)) {
	return true;
      }
    } else {
      for (pt = *l; pt != nil && pt->snext != p; pt = pt->snext)
	;

      if (pt == nil) {
	return false;
      } else if (cas(&pt->snext, p, p->snext)) {
	return true;
      }
    }
  }
}
