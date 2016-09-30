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

  /* Need to get this working properly */

  if (!testandset(&l->lock)) {
    disableintr();
    while (!testandset(&l->listlock)) {
      schedule();
    }

    if (testandset(&l->lock)) {
      l->listlock = 0;
    } else {
      for (p = l->waiting; p != nil && p->wnext != nil; p = p->wnext);
      if (p == nil) {
	l->waiting = current;
      } else {
	p->wnext = current;
      }
      current->wnext = nil;

      printf("%i waiting on lock 0x%h\n", current->pid, l);
      l->listlock = 0;
      procwait(current);
      schedule();
    }
    enableintr();
  }

  l->holder = current;
}

void
unlock(struct lock *l)
{
  struct proc *p;
  l->lock = 0;

  return;

  /* Need to get this working. Not sure why it doesnt */
  

  p = l->waiting;
  if (p != nil) {
    l->waiting = p->wnext;
    procready(p);
  }

  l->holder = nil;
  l->listlock = 0;
  l->lock = 0;
  enableintr();
}
