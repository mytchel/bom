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

struct agroup *
agroupnew(size_t max)
{
  struct agroup *g;

  g = malloc(sizeof(struct agroup));
  if (g == nil) {
    return nil;
  }

  g->refs = 1;
  g->naddrs = max;

  g->addrs = malloc(sizeof(struct addr *) * max);
  if (g->addrs == nil) {
    free(g);
    return nil;
  }
  
  return g;
}

struct agroup *
agroupcopy(struct agroup *o)
{
  struct agroup *g;
  int i;
  
  g = malloc(sizeof(struct agroup));
  if (g == nil) {
    return nil;
  }

  g->refs = 1;
  g->naddrs = o->naddrs;

  g->addrs = malloc(sizeof(struct addr *) * g->naddrs);
  if (g->addrs == nil) {
    free(g);
    return nil;
  }

  for (i = 0; i < o->naddrs; i++) {
    g->addrs[i] = o->addrs[i];
    if (g->addrs[i] != nil) {
      atomicinc(&g->addrs[i]->refs);
    }
  }
  
  return g;
}

void
agroupfree(struct agroup *g)
{
  int a;
  
  if (atomicdec(&g->refs) > 0) {
    return;
  }

  for (a = 0; a < g->naddrs; a++) {
    if (g->addrs[a] != nil) {
      addrfree(g->addrs[a]);
    }
  }
  
  free(g->addrs);
  free(g);
}

struct addr *
addrnew(void)
{
  struct addr *a;

  a = malloc(sizeof(struct addr));
  if (a == nil) {
    return nil;
  }

  a->refs = 1;
  a->nextmid = 1;
  
  a->recv = a->reply = nil;
  a->waiting = nil;

  return a;
}

void
addrfree(struct addr *a)
{
  struct message *m;
  
  if (atomicdec(&a->refs) > 0) {
    return;
  }

  for (m = a->recv; m != nil; m = m->next) {
    m->ret = ERR;
    procready(m->sender);
  }    

  for (m = a->reply; m != nil; m = m->next) {
    m->ret = ERR;
    procready(m->sender);
  }

  free(a);
}

int
kmessage(struct addr *a, struct message *m)
{
  struct message *p;
 
  m->mid = atomicinc(&a->nextmid);
  m->sender = up;
  m->replyer = nil;

  m->next = nil;
  while (true) {
    p = a->recv;
    while (p != nil && p->next != nil)
      p = p->next;

    if (p == nil) {
      if (cas(&a->recv, nil, m)) {
	break;
      }
    } else if (cas(&p->next, nil, m)) {
      break;
    }
  }

  if (a->waiting != nil) {
    procready(a->waiting);
  }

  procwait();

  return m->ret;
}

struct message *
krecv(struct addr *a)
{
  struct message *m;

  lock(&a->lock);
  
  do {
    m = a->recv;

    if (m == nil) {
      a->waiting = up;
      procwait();
      continue;
    }

  } while (!cas(&a->recv, m, m->next));
  
  unlock(&a->lock);

  m->next = a->reply;
  a->reply = m;
  
  return m;
}

int
kreply(struct addr *a, uint32_t mid, void *rb)
{
  struct message *m, *p;

 retry:
  p = nil;
  for (m = a->reply; m != nil; p = m, m = m->next) {
    if (m->mid != mid) continue;

    if (p == nil) {
      if (!cas(&a->reply, m, m->next)) {
	goto retry;
      }
    } else if (!cas(&p->next, m, m->next)) {
      goto retry;
    }

    break;
  }

  if (m == nil) {
    return ERR;
  }
  
  memmove(m->reply, rb, MESSAGELEN);
  m->replyer = up;
  m->ret = OK;

  procready(m->sender);
  
  return OK;
}
