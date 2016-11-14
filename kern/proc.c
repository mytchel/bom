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
static struct proc *waitchildren = nil;
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
    ready = p->next;
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
	sleeping = p->next;
      } else {
	pp->next = p->next;
      }

      pt = p->next;

      p->state = PROC_ready;
      addtolistfront(&ready, p);

      p = pt;
    } else {
      p->sleep -= t;

      pp = p;
      p = p->next;
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

  memset(p, 0, sizeof(struct proc));

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
  struct proc *c;
  
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

  p->state = PROC_dead;
  p->exitcode = code;
 
  while (p->parent != nil && p->parent->state == PROC_dead) {
    c = p->parent;
    p->parent = c->parent;

    if (atomicdec(&c->nchildren) == 0) {
      procfree(c);
    }
  }

  if (p->parent != nil) {
    addtolistback(&p->parent->deadchildren, p);
    atomicdec(&p->parent->nchildren);

    if (p->parent->state == PROC_waiting) {
      if (removefromlist(&waitchildren, p->parent)) {
	procready(p->parent);
      }
    }
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
    if (up->nchildren == 0) {
      return nil;
    } else {
      addtolistfront(&waitchildren, up);
      procwait(up);

      i = setintr(INTR_OFF);
      schedule();
      setintr(i);
    }
  }

  do {
    p = up->deadchildren;
  } while (!(cas(&up->deadchildren, p, p->next)));

  return p;
}

void
procready(struct proc *p)
{
  p->state = PROC_ready;

  if (p->state == PROC_suspend) {
    removefromlist(&suspended, p);
  }

  addtolistfront(&ready, p);
}

void
procsleep(struct proc *p, uint32_t ms)
{
  p->state = PROC_sleeping;
  p->sleep = mstoticks(ms);
  
  addtolistfront(&sleeping, p);
}

void
procyield(struct proc *p)
{
  p->state = PROC_sleeping;
  addtolistback(&ready, p);
}

void
procsuspend(struct proc *p)
{
  p->state = PROC_suspend;
  addtolistfront(&suspended, p);
}

void
procwait(struct proc *p)
{
  p->state = PROC_waiting;
}

void
addtolistfront(struct proc **l, struct proc *p)
{
  do {
    p->next = *l;
  } while (!cas(l, p->next, p));
}

void
addtolistback(struct proc **l, struct proc *p)
{
  struct proc *pp;

  p->next = nil;
  
  while (true) {
    for (pp = *l; pp != nil && pp->next != nil; pp = pp->next)
      ;

    if (pp == nil) {
      if (cas(l, nil, p)) {
	break;
      }
    } else if (cas(&pp->next, nil, p)) {
      break;
    }
  }
}

bool
removefromlist(struct proc **l, struct proc *p)
{
  struct proc *pp, *pt;

  while (true) {
    pp = nil;
    for (pt = *l; pt != nil && pt != p; pp = pt, pt = pt->next)
      ;

    if (pt != p) {
      return false;
    } else if (pp == nil) {
      if (cas(l, p, p->next)) {
	return true;
      }
    } else {
      if (cas(&pp->next, p, p->next)) {
	return true;
      }
    }
  }
}
