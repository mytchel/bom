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

struct pipe {
  struct lock lock;
  
  struct chan *c0, *c1;

  bool waiting;	
  struct proc *proc;
  void *buf;
  size_t n;
};

bool
pipenew(struct chan **c0, struct chan **c1)
{
  struct pipe *p;
	
  p = malloc(sizeof(struct pipe));
  if (p == nil) {
    return false;
  }
	
  *c0 = channew(CHAN_pipe, O_RDONLY);
  if (*c0 == nil) {
    free(p);
    return false;
  }
	
  *c1 = channew(CHAN_pipe, O_WRONLY);
  if (*c1 == nil) {
    chanfree(*c0);
    free(p);
    return false;
  }
	
  (*c0)->aux = p;
  (*c1)->aux = p;
	
  p->c0 = *c0;
  p->c1 = *c1;
  p->waiting = false;
  p->proc = nil;

  return true;	
}

static int
pipedocopy(struct pipe *p, void *buf, size_t n, bool writing)
{
  intrstate_t i;
  int r;

  lock(&p->lock);

  if (p->c0 == nil || p->c1 == nil) {
    r = EOF;
    unlock(&p->lock);
  } else if (p->waiting) {
    /* Do copy now */

    r = n > p->n ? p->n : n;
    if (writing) {
      memmove(p->buf, buf, r);
    } else {
      memmove(buf, p->buf, r);
    }

    p->proc->aux = (void *) r;
    procready(p->proc);
    p->waiting = false;

    unlock(&p->lock);
  } else {
    /* Wait for other end to do copy */
    
    p->buf = buf;
    p->n = n;
    p->waiting = true;
    p->proc = up;
    
    i = setintr(INTR_OFF);

    unlock(&p->lock);
    procwait(up);
    schedule();

    setintr(i);

    r = (int) up->aux;
  }

  return r;
}

int
piperead(struct chan *c, void *buf, size_t n)
{
  struct pipe *p;
	
  p = (struct pipe *) c->aux;

  return pipedocopy(p, buf, n, false);
}

int
pipewrite(struct chan *c, void *buf, size_t n)
{
  struct pipe *p;
	
  p = (struct pipe *) c->aux;

  return pipedocopy(p, buf, n, true);
}

void
pipeclose(struct chan *c)
{
  struct pipe *p;
	
  p = (struct pipe *) c->aux;

  lock(&p->lock);

  if (c == p->c0) {
    p->c0 = nil;
  } else {
    p->c1 = nil;
  }
	
  if (p->waiting) {
    p->proc->aux = (void *) EOF;
    procready(p->proc);
    p->waiting = false;
  }

  if (p->c0 == nil && p->c1 == nil) {
    free(p);
  } else {
    unlock(&p->lock);
  }
}

int
pipeseek(struct chan *c, size_t offset, int whence)
{
  return ENOIMPL;
}

struct chantype devpipe = {
  &piperead,
  &pipewrite,
  &pipeseek,
  &pipeclose,
};
