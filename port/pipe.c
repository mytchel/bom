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

  initlock(&p->lock);

  return true;	
}

static int
pipedocopy(struct pipe *p, uint8_t *buf, size_t n, bool writing)
{
  int r;

  lock(&p->lock);

  if (p->c0 == nil || p->c1 == nil) {
    r = ELINK;
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
    p->waiting = false;

    procready(p->proc);
    unlock(&p->lock);
  } else {
    /* Wait for other end to do copy */
    
    p->buf = buf;
    p->n = n;
    p->waiting = true;

    disableintr();
    procwait(current, &p->proc);
    unlock(&p->lock);
    schedule();
    enableintr();

    r = (int) current->aux;
  }

  return r;
}

int
piperead(struct chan *c, uint8_t *buf, size_t n)
{
  struct pipe *p;
	
  p = (struct pipe *) c->aux;

  return pipedocopy(p, buf, n, false);
}

int
pipewrite(struct chan *c, uint8_t *buf, size_t n)
{
  struct pipe *p;
	
  p = (struct pipe *) c->aux;

  return pipedocopy(p, buf, n, true);
}

int
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
    p->proc->aux = (void *) ELINK;
    disableintr();
    p->waiting = false;
    procready(p->proc);
    enableintr();
  }

  if (p->c0 == nil && p->c1 == nil) {
    free(p);
  } else {
    unlock(&p->lock);
  }
	
  return 0;
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
