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

static struct proc *
nextproc(void);

static uint32_t nextpid = 1;
static struct proc procs[MAX_PROCS] = {{0}};

struct proc *current = &procs[0];

void
schedule(void)
{
  if (setlabel(&current->label)) {
    return;
  }
	
  current = nextproc();
  setsystick(current->quanta);
  mmuswitch(current);
  gotolabel(&current->label);
}

struct proc *
nextproc(void)
{
  struct proc *p;
  uint32_t t;

  t = ticks();
	
  for (p = procs->next; p != nil; p = p->next) {
    if (p->state == PROC_sleeping) {
      if (t >= p->sleep) {
	p->state = PROC_ready;
      } else {
	p->sleep -= t;
      }
    }
  }

  p = current;
  do {
    p = p->next;
    if (p == nil) {
      p = procs->next;
    }

    if (p->state == PROC_ready) {
      break;
    }

  } while (p != current);

  if (p->state != PROC_ready) {
    panic("No process to run!\n");
  }

  return p;
}
	
struct proc *
newproc(void)
{
  int i;
  struct proc *p, *pp;
	
  p = nil;

  disableintr();
  for (i = 1; i < MAX_PROCS; i++) {
    if (procs[i].state == PROC_unused) {
      p = &procs[i];
      p->state = PROC_suspend;
      break;
    }
  }

  enableintr();

  if (p == nil)
    return nil;
	
  p->faults = 0;
  p->ureg = nil;
  p->wnext = nil;
  p->inkernel = true;

  p->quanta = 10;

  p->pid = nextpid++;
  p->parent = nil;
  p->dot = nil;

  p->stack = nil;
  p->mmu = nil;

  p->mgroup = nil;
  p->fgroup = nil;
  p->ngroup = nil;

  disableintr();

  for (pp = procs; pp->next; pp = pp->next);
  pp->next = p;
  p->next = nil;

  enableintr();
		
  return p;
}

void
procremove(struct proc *p)
{
  struct proc *pp;

  /* Remove proc from list. */
  for (pp = procs; pp != nil && pp->next != p; pp = pp->next);
  if (pp == nil) /* Umm */
    panic("proc remove somehow didn't find previous proc!\n");
  
  pp->next = p->next;

  freepath(p->dot);

  freepagel(p->stack);
  freepagel(p->mmu);

  if (p->mgroup != nil)
    freemgroup(p->mgroup);

  if (p->fgroup != nil)
    freefgroup(p->fgroup);

  if (p->ngroup != nil)
    freengroup(p->ngroup);
	
  p->state = PROC_unused;
}

void
procready(struct proc *p)
{
  p->state = PROC_ready;
}

void
procsleep(struct proc *p, uint32_t ms)
{
  p->state = PROC_sleeping;
  p->sleep = mstoticks(ms);
}

void
procsuspend(struct proc *p)
{
  p->state = PROC_suspend;
}

void
procwait(struct proc *p)
{
  p->state = PROC_waiting;
}
