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
lock(struct lock *l)
{
  intrstate_t i;
  
  if (!cas(&l->holder, nil, up)) {
    up->next = l->wlist;
    if (!cas(&l->wlist, up->next, up)) {
      return lock(l);
    }
      
    procwait(up);
    i = setintr(INTR_OFF);
    schedule();
    setintr(i);
  }
}

void
unlock(struct lock *l)
{
  struct proc *p, *pp;

  pp = nil;
  for (p = l->wlist; p != nil && p->next != nil; pp = p, p = p->next)
    ;

  if (p != nil) {
    if (pp == nil) {
      l->wlist = nil;
    } else {
      pp->next = nil;
    }
    
    l->holder = p; 
    procready(p);
  } else {
    l->holder = nil;
  }
}
