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
  while (!testandset(&(l->lock)))
    schedule();

  return;
  
  if (!testandset(&(l->lock))) {
    while (!testandset(&(l->listlock)))
      schedule();

    if (testandset(&(l->lock))) {
      l->listlock = 0;
    } else {
      current->wnext = l->waiting;
      l->waiting = current;

      disableintr();
      l->listlock = 0;
      procwait(current);
      schedule();
      enableintr();
    }
  }
}

void
unlock(struct lock *l)
{
  struct proc *w;
  
  while (!testandset(&(l->listlock)))
    schedule();

  w = l->waiting;
  if (w != nil) {
    l->waiting = w->wnext;
    procready(w);
  }

  l->listlock = 0;
  l->lock = 0;
}
