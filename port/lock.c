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
initlock(struct lock *l)
{
  l->lock = 0;
  l->listlock = 0;
  l->waiting = nil;
}

void
lock(struct lock *l)
{
  struct proc *p;

  while (!testandset(&l->lock)) {
    disableintr();
    schedule();
    enableintr();
  }

  return;

  printf("%i lock 0x%h\n", current->pid, l);
  if (!testandset(&l->lock)) {
    printf("%i failed to get 0x%h\n", current->pid, l);

    disableintr();
    while (!testandset(&l->listlock)) {
      printf("%i failed to get 0x%h list lock\n", current->pid, l);
      schedule();
    }

    if (testandset(&l->lock)) {
      printf("%i ended up getting 0x%h\n", current->pid, l);
      l->listlock = 0;
    } else {
      printf("%i adding self to 0x%h wlist\n", current->pid, l);
      for (p = l->waiting; p != nil && p->next != nil; p = p->wnext);
      if (p == nil) {
	l->waiting = current;
      } else {
	p->wnext = current;
      }
      current->wnext = nil;

      printf("%i sleep\n", current->pid, l);
      l->listlock = 0;
      procwait(current);
      schedule();
    }
    enableintr();
  }
}

void
unlock(struct lock *l)
{
  struct proc *p;

  l->lock = 0;
  return;
  
  printf("%i unlock 0x%h\n", current->pid, l);
  disableintr();
  while (!testandset(&l->listlock)) {
    printf("%i failed to get 0x%h listlock\n", current->pid, l);
    schedule();
  }

  printf("%i unlocking 0x%h \n", current->pid, l);
  p = l->waiting;
  if (p != nil) {
    printf("%i wake %i \n", current->pid, p->pid);
    l->waiting = p->wnext;
    procready(p);
  }

  printf("%i unlocked \n", current->pid);

  l->listlock = 0;
  l->lock = 0;
  enableintr();
}
