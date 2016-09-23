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

struct pipe {
  struct chan *c0, *c1;

  bool waiting;	
  struct proc *proc;
  uint8_t *buf;
  size_t n;
};

bool
newpipe(struct chan **c0, struct chan **c1)
{
  struct pipe *p;
	
  p = malloc(sizeof(struct pipe));
  if (p == nil) {
    return false;
  }
	
  *c0 = newchan(CHAN_pipe, O_RDONLY, nil);
  if (*c0 == nil) {
    free(p);
    return false;
  }
	
  *c1 = newchan(CHAN_pipe, O_WRONLY, nil);
  if (*c1 == nil) {
    freechan(*c0);
    free(p);
    return false;
  }
	
  (*c0)->aux = p;
  (*c1)->aux = p;
	
  p->c0 = *c0;
  p->c1 = *c1;
  p->waiting = false;

  return true;	
}

int
piperead(struct chan *c, uint8_t *buf, size_t n)
{
  struct pipe *p;
	
  p = (struct pipe *) c->aux;

  if (p->c0 == nil || p->c1 == nil) {
    return ELINK;
  }

  p->waiting = true;
  p->proc = current;
  p->buf = buf;
  p->n = n;

  disableintr();
  procwait(current);
  schedule();
  enableintr();

  if (p->n != 0) {
    return n - p->n;
  } else {
    return n;
  }
}

int
pipewrite(struct chan *c, uint8_t *buf, size_t n)
{
  size_t l, t;
  struct pipe *p;
	
  p = (struct pipe *) c->aux;

  if (p->c0 == nil || p->c1 == nil) {
    return ELINK;
  }

  t = 0;
  while (t < n) {
    while (!p->waiting && p->c0 != nil && p->c1 != nil) {
      schedule();
    }

    if (p->c0 == nil || p->c1 == nil) {
      return t;
    }

    disableintr();
    l = n - t < p->n ? n - t : p->n;
    memmove(p->buf, buf, l);
    enableintr();
		
    t += l;
    buf += l;
		
    if (l == p->n) {
      p->n = 0;
      p->waiting = false;
      procready(p->proc);
    } else {
      p->n -= l;
      p->buf += l;
    }
  }
	
  return t;
}

int
pipeclose(struct chan *c)
{
  struct pipe *p;
	
  p = (struct pipe *) c->aux;

  if (c == p->c0) {
    p->c0 = nil;
  } else {
    p->c1 = nil;
  }
	
  if (p->waiting) {
    procready(p->proc);
    p->waiting = false;
  }

  if (p->c0 == nil && p->c1 == nil) {
    free(p);
  }
	
  return 0;
}

struct chantype devpipe = {
  &piperead,
  &pipewrite,
  &pipeclose,
};
