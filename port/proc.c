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

static void addtolist(struct proc **, struct proc *);
static void removefromlist(struct proc **, struct proc *);

struct priority_queue {
  uint32_t quanta;
  struct proc *ready;
  struct proc *used;
};

struct proc *current = nil;

static uint32_t nextpid = 1;

static struct priority_queue priorities[MIN_PRIORITY + 2];

static struct proc *sleeping = nil;
static struct proc *suspended = nil;
static struct proc *nullproc;

void
initscheduler(void)
{
  int i;

  for (i = 1; i <= MIN_PRIORITY; i++) {
    priorities[i].quanta = 300 - i;
    priorities[i].ready = nil;
    priorities[i].used = nil;
  }

  priorities[MIN_PRIORITY+1].quanta = 10;
  priorities[MIN_PRIORITY+1].ready = nil;
  priorities[MIN_PRIORITY+1].used = nil;

  nullproc = newproc(MIN_PRIORITY+1);
  if (nullproc == nil) {
    panic("Failed to create null proc!\n");
  }
  
  forkfunc(nullproc, &nullprocfunc, nil);
  procready(nullproc);
  current = nullproc;
}

static struct proc *
nextproc()
{
  struct proc *p;
  unsigned int i;

  for (i = 1; i <= MIN_PRIORITY; i++) {
    p = priorities[i].ready;
    if (p != nil) {
      priorities[i].ready = p->next;
      p->list = nil;
      return p;
    }
  }

  for (i = 1; i <= MIN_PRIORITY; i++) {
    priorities[i].ready = priorities[i].used;
    priorities[i].used = nil;

    p = priorities[i].ready;
    if (p != nil) {
      priorities[i].ready = p->next;
      p->list = nil;
      return p;
    }
  }

  return nullproc;
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
      addtolist(&priorities[p->priority].ready, p);

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

  if (setlabel(&current->label)) {
    return;
  }

  t = ticks();

  current->timeused += t;
  if (current->state == PROC_ready) {
    if (current->timeused >= priorities[current->priority].quanta) {
      current->timeused = 0;
      addtolist(&priorities[current->priority].used, current);
    } else {
      addtolist(&priorities[current->priority].ready, current);
    }
  }

  updatesleeping(t);
	
  current = nextproc();
  mmuswitch(current);
  setsystick(priorities[current->priority].quanta - current->timeused);
  gotolabel(&current->label);
}
	
struct proc *
newproc(unsigned int priority)
{
  struct proc *p;
	
  p = malloc(sizeof(struct proc));
  if (p == nil) {
    return nil;
  }
	
  p->priority = priority;
  
  p->faults = 0;
  p->ureg = nil;
  p->inkernel = true;

  p->timeused = 0;
  
  p->pid = nextpid++;
  p->parent = nil;

  p->dot = nil;
  p->dotchan = nil;

  p->stack = nil;
  p->mmu = nil;

  p->mgroup = nil;
  p->fgroup = nil;
  p->ngroup = nil;

  p->state = PROC_suspend;
  addtolist(&suspended, p);

  return p;
}

void
procremove(struct proc *p)
{
  removefromlist(p->list, p);

  freechan(p->dotchan);
  freepath(p->dot);

  freepagel(p->stack);
  freepagel(p->mmu);

  if (p->mgroup != nil)
    freemgroup(p->mgroup);

  if (p->fgroup != nil)
    freefgroup(p->fgroup);

  if (p->ngroup != nil)
    freengroup(p->ngroup);

  free(p);
}

void
procsetpriority(struct proc *p, unsigned int priority)
{
  p->priority = priority;
  
  if (p->state == PROC_ready) {
    removefromlist(p->list, p);
    addtolist(&priorities[p->priority].ready, p);
  }
}

void
procready(struct proc *p)
{
  p->state = PROC_ready;
  removefromlist(p->list, p);
  addtolist(&priorities[p->priority].ready, p);
}

void
procsleep(struct proc *p, uint32_t ms)
{
  p->state = PROC_sleeping;
  p->sleep = mstoticks(ms);

  removefromlist(p->list, p);
  addtolist(&sleeping, p);
}

void
procsuspend(struct proc *p)
{
  p->state = PROC_suspend;

  removefromlist(p->list, p);
  addtolist(&suspended, p);
}

void
procwait(struct proc *p, struct proc **wlist)
{
  p->state = PROC_waiting;

  removefromlist(p->list, p);
  addtolist(wlist, p);
}

void
addtolist(struct proc **l, struct proc *p)
{
  p->list = l;
  p->next = *l;
  *l = p;
}

void
removefromlist(struct proc **l, struct proc *p)
{
  struct proc *pp, *pt;

  if (l == nil) {
    return;
  }
  
  pp = nil;
  for (pt = *l; pt != p; pp = pt, pt = pt->next)
    ;

  if (pp == nil) {
    *l = p->next;
  } else {
    pp->next = p->next;
  }
}
