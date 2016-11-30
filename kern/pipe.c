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

struct pipetrans {
  struct proc *proc;
  void *buf;
  size_t n;
  bool writing;
  struct pipetrans *next;
};

struct pipe {
  struct chan *c0, *c1;
  struct pipetrans *trans;
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
  p->trans = nil;

  return true;	
}

static int
pipedocopy(struct pipe *p, void *buf, size_t n, bool writing)
{
  struct pipetrans *t, *h, *tp, nt;
  int r;

  h = p->trans;

  tp = nil;
  for (t = h; t != nil && t->next != nil; tp = t, t = t->next)
    ;
  
  if (p->c0 == nil || p->c1 == nil) {
    return EOF;
  } else if (t == nil || t->writing == writing) {
    nt.proc = up;
    nt.buf = buf;
    nt.n = n;
    nt.writing = writing;

    nt.next = h;

    if (!cas(&p->trans, h, &nt)) {
      return pipedocopy(p, buf, n, writing);
    }
    
    procwait();

    return nt.n;
   
  } else {
    /* Do copy now */
    if (tp == nil) {
      p->trans = nil;
    } else {
      tp->next = nil;
    }
    
    r = n > t->n ? t->n : n;
    if (writing) {
      memmove(t->buf, buf, r);
    } else {
      memmove(buf, t->buf, r);
    }

    t->n = r;
    procready(t->proc);

    return r;
  }
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
  struct pipetrans *t, *tp;
  struct pipe *p;
	
  p = (struct pipe *) c->aux;

  if (c == p->c0) {
    p->c0 = nil;
  } else {
    p->c1 = nil;
  }

  tp = p->trans;
  p->trans = nil;
  
  for (t = tp; t != nil; t = t->next) {
    t->n = EOF;
    procready(t->proc);
  }
  
  if (p->c0 == nil && p->c1 == nil) {
    free(p);
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
