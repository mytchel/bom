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

static struct proc *
nextproc(void);

static uint32_t nextpid = 1;
static struct proc procs[MAX_PROCS] = {{0}};

struct proc *current = procs;

void
schedule(void)
{
  /* gotolabel will enable on switch. */
  disableintr();
	
  if (setlabel(&current->label)) {
    return;
  }
	
  current = nextproc();
  debug("schedule %i\n", current->pid);
  mmuswitch(current);
  setsystick(current->quanta);
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
      if (t > p->sleep) {
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
	
  return p;
}
	
struct proc *
newproc(void)
{
  int i;
  struct proc *p, *pp;
	
  disableintr();
  p = nil;
  for (i = 1; i < MAX_PROCS; i++) {
    if (procs[i].state == PROC_unused) {
      p = &procs[i];
      break;
    }
  }
  enableintr();

  if (p == nil)
    return nil;
	
  p->state = PROC_suspend;
  p->pid = nextpid++;
  p->faults = 0;
  p->ureg = nil;
  p->wnext = nil;
  p->mmu = nil;
  p->inkernel = true;

  for (pp = procs; pp->next; pp = pp->next);
  pp->next = p;
  p->next = nil;
		
  return p;
}

void
initproc(struct proc *p)
{
  p->dot = nil;
  p->quanta = 100;
	
  p->segs[Sstack] = newseg(SEG_rw);
  p->segs[Stext] = newseg(SEG_ro);
  p->segs[Sdata] = newseg(SEG_rw);
  p->segs[Sheap] = newseg(SEG_rw);
	
  p->fgroup = nil;
  p->ngroup = nil;
}

void
procremove(struct proc *p)
{
  struct proc *pp;
  int i;

  debug("proc %i remove\n", p->pid);

  disableintr();
  /* Remove proc from list. */
  for (pp = procs; pp != nil && pp->next != p; pp = pp->next);
  if (pp == nil) /* Umm */
    return;
  pp->next = p->next;

  enableintr();

  for (i = 0; i < Smax; i++) {
    freeseg(p->segs[i]);
  }
	
  freepath(p->dot);
  freefgroup(p->fgroup);
  freengroup(p->ngroup);
	
  p->state = PROC_unused;
}

void
procready(struct proc *p)
{
  debug("proc %i ready\n", p->pid);
  p->state = PROC_ready;
}

void
procsleep(struct proc *p, uint32_t ms)
{
  debug("proc %i sleep\n", p->pid);
  p->state = PROC_sleeping;
  p->sleep = mstoticks(ms);
	
  if (p == current)
    schedule();
}

void
procsuspend(struct proc *p)
{
  debug("proc %i suspend\n", p->pid);
  p->state = PROC_suspend;
}

void
procwait(struct proc *p)
{
  debug("proc %i wait\n", p->pid);
  p->state = PROC_waiting;
}
