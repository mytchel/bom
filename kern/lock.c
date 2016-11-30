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
  struct proc *p;
  
  if (up == nil) {
    return;
  }

 retry:

  if (!cas(&l->holder, nil, up)) {
    p = l->wlist;
    while (p != nil && p->next != nil)
      p = p->next;

    up->next = nil;

    if (p == nil) {
      if (!cas(&l->wlist, nil, up)) {
	goto retry;
      }
    } else if (!cas(&p->next, nil, up)) {
      goto retry;
    }

    procwait();
  }
}

void
unlock(struct lock *l)
{
  struct proc *p;

  if (up == nil) {
    return;
  }

 retry:

  p = l->wlist;
  if (p != nil) {
    if (!cas(&l->wlist, p, p->next)) {
      goto retry;
    }

    p->next = nil;
    
    l->holder = p; 
    procready(p);
  } else {
    l->holder = nil;
  }
}
