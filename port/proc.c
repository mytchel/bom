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
static void removefromlist(struct proc **, struct proc *);

struct priority_queue {
  uint32_t quanta;
  struct proc *ready;
  struct proc *used;
};

static uint32_t nextpid = 1;

/*
static struct priority_queue priorities[MIN_PRIORITY + 2];
*/

static struct proc *sleeping = nil;
static struct proc *suspended = nil;
static struct proc *nullproc = nil;

static struct proc *ready = nil;

struct proc *current = nil;

void
initscheduler(void)
{
 /*
 int i;
  
  for (i = 1; i <= MIN_PRIORITY; i++) {
    priorities[i].quanta = mstoticks(MIN_PRIORITY - i + 10);
    priorities[i].ready = nil;
    priorities[i].used = nil;
  }

  priorities[MIN_PRIORITY+1].quanta = mstoticks(10);
  priorities[MIN_PRIORITY+1].ready = nil;
  priorities[MIN_PRIORITY+1].used = nil;
*/
  nullproc = newproc(MIN_PRIORITY+1);
  if (nullproc == nil) {
    panic("Failed to create null proc!\n");
  }
  
  forkfunc(nullproc, &nullprocfunc, nil);
  procready(nullproc);
}

static struct proc *
nextproc()
{
  struct proc *p;

  if (ready != nil) {
    p = ready;
    ready = p->next;
    p->timeused = 0;
    return p;
  } else {
    nullproc->timeused = 0;
    return nullproc;
  }

  /*
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

  nullproc->timeused = 0;
  return nullproc;
  */
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
      /*
      addtolistfront(&priorities[p->priority].ready, p);
      */
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

  if (current && setlabel(&current->label)) {
    return;
  }

  t = ticks();

  if (current != nil) {
    current->timeused += t;

    if (current->state == PROC_oncpu) {
      current->state = PROC_ready;

      current->timeused = 0;
      addtolistback(&ready, current);
      /*
      if (current->timeused >= priorities[current->priority].quanta) {

	current->timeused = 0;
	addtolistback(&priorities[current->priority].used, current);
      } else {
	addtolistback(&priorities[current->priority].ready, current);
      }
      */
    }
  }
  
  updatesleeping(t);
	
  current = nextproc();
  current->state = PROC_oncpu;

  if (current->pid != 1) {
    printf("run %i\n", current->pid);
  }
  
  mmuswitch(current);

  setsystick(40);
  /*priorities[current->priority].quanta
	     - current->timeused);
  */
  gotolabel(&current->label);
}
	
struct proc *
newproc(unsigned int priority)
{
  struct proc *p;
  struct page *pg;
	
  p = malloc(sizeof(struct proc));
  if (p == nil) {
    return nil;
  }
	
  p->pid = nextpid++;
  p->priority = priority;
  
  p->ureg = nil;
  p->inkernel = true;

  p->timeused = 0;
  
  p->parent = nil;

  p->dot = nil;
  p->dotchan = nil;

  pg = getrampage();
  if (pg == nil) {
    free(p);
    return nil;
  }
  
  p->kstack = wrappage(pg, (void *) (KSTACK_TOP - PAGE_SIZE),
		       true, true);
  if (p->kstack == nil) {
    freepage(pg);
    free(p);
    return nil;
  }

  p->ustack = nil;
  p->mmu = nil;

  p->mgroup = nil;
  p->fgroup = nil;
  p->ngroup = nil;

  p->state = PROC_suspend;
  addtolistfront(&suspended, p);

  return p;
}

void
procremove(struct proc *p)
{
  printf("proc remove %i\n", p->pid);

  if (p == current) {
    printf("reset current\n");
    current = nil;
  }

  printf("remove from list\n");
  removefromlist(p->list, p);

  printf("free chan and dot\n");
  freepath(p->dot);
  if (p->dotchan != nil)
    freechan(p->dotchan);

  printf("free kstack\n");
  freepagel(p->kstack);
  printf("free ustack\n");
  freepagel(p->ustack);
  printf("free mmu\n");
  freepagel(p->mmu);

  printf("free mgroup\n");
  if (p->mgroup != nil)
    freemgroup(p->mgroup);

  printf("free fgroup\n");
  if (p->fgroup != nil)
    freefgroup(p->fgroup);

  printf("free ngroup\n");
  if (p->ngroup != nil)
    freengroup(p->ngroup);

  printf("free proc\n");
  free(p);
  printf("proc removed\n");
}

void
procsetpriority(struct proc *p, unsigned int priority)
{
  p->priority = priority;

  /*
  if (p->state == PROC_ready) {
    removefromlist(p->list, p);
    addtolistfront(&priorities[p->priority].ready, p);
  }
  */
}

void
procready(struct proc *p)
{
  p->state = PROC_ready;

  removefromlist(p->list, p);
  addtolistfront(&ready, p);

  /*
  if (p->timeused < priorities[p->priority].quanta) {
    addtolistfront(&priorities[p->priority].ready, p);
  } else {
    p->timeused = 0;
    addtolistback(&priorities[p->priority].used, p);
  }
  */
}

void
procsleep(struct proc *p, uint32_t ms)
{
  p->state = PROC_sleeping;
  p->sleep = mstoticks(ms);
  
  removefromlist(p->list, p);
  addtolistfront(&sleeping, p);
}

void
procyield(struct proc *p)
{
  p->state = PROC_sleeping;
  p->timeused = 0;

  removefromlist(p->list, p);
  addtolistback(&ready, p);
  /*
  addtolistback(&priorities[p->priority].used, p);
  */
}

void
procsuspend(struct proc *p)
{
  p->state = PROC_suspend;

  removefromlist(p->list, p);
  addtolistfront(&suspended, p);
}

void
procwait(struct proc *p, struct proc **wlist)
{
  p->state = PROC_waiting;

  removefromlist(p->list, p);
  addtolistback(wlist, p);
}

void
addtolistfront(struct proc **l, struct proc *p)
{
  p->list = l;
  p->next = *l;
  *l = p;
}

void
addtolistback(struct proc **l, struct proc *p)
{
  struct proc *pp;

  p->next = nil;
  p->list = l;

  if (*l == nil) {
    *l = p;
  } else {
    for (pp = *l; pp->next != nil; pp = pp->next)
      ;
    pp->next = p;
  }
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

  p->list = nil;
}
