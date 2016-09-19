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
  printf("%i locking\n", current->pid);
  if (testandset(&(l->lock))) {
    printf("got lock\n");
    return;
  } else {
    printf("%i adding to a lock waiter\n", current->pid);
    while (!testandset(&(l->listlock)))
      schedule();

    if (testandset(&(l->lock))) {
      printf("got lock in the end\n");
      l->listlock = 0;
    } else {
      printf("waiting for lock\n");

      current->wnext = l->waiting;
      l->waiting = current;

      disableintr();
      procwait(current);
      l->listlock = 0;
      schedule();
      enableintr();
      printf("%i back from lock wait\n", current->pid);
    }
  }
}

void
unlock(struct lock *l)
{
  printf("unlock, %i get list lock\n", current->pid);
  while (!testandset(&(l->listlock)))
    schedule();

  if (l->waiting != nil) {
    printf("wake up first in list (%i)\n", l->waiting->pid);
    procready(l->waiting);
    l->waiting = l->waiting->next;
  }

  l->listlock = 0;
  l->lock = 0;
}
