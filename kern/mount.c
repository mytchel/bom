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

int
mountproc(void *arg)
{
  struct fstransaction *t, *p;
  struct response tresp;
  struct binding *b;
  
  b = (struct binding *) arg;

  while (b->refs > 1) {
    if (piperead(b->in, (void *) &tresp, sizeof(tresp)) < 0) {
      break;
    }

  findandremove:
    p = nil;
    for (t = b->waiting; t != nil; p = t, t = t->next) {
      if (t->rid != tresp.head.rid) {
	continue;
      } else if (p == nil) {
	if (!cas(&b->waiting, t, t->next)) {
	  goto findandremove;
	}
      } else {
	if (!cas(&p->next, t, t->next)) {
	  goto findandremove;
	}
      }

      break;
    }

    if (t == nil) {
      printf("kproc mount: response has no waiter.\n");
      break;
    } 

    memmove(t->resp, &tresp, t->len);

    if (t->type == REQ_read && t->resp->head.ret == OK) {
      t->resp->read.len = piperead(b->in, t->resp->read.data,
				   t->resp->read.len);

      if (t->resp->read.len < 0) {
	printf("kproc mount: error reading read response.\n");
	break;
      }
    }

    /* To make sure requesting proc is waiting,
       need a better way to do this.
    */
    lock(&b->lock);
    unlock(&b->lock);

    procready(t->proc);
  }

  lock(&b->lock);

  if (b->in != nil) {
    chanfree(b->in);
    b->in = nil;
  }

  if (b->out != nil) {
    chanfree(b->out);
    b->out = nil;
  }

  /* Wake up any waiting processes so they can error. */

  t = b->waiting;
  while (t != nil) {
    p = t->next;
    
    t->resp->head.ret = ELINK;
    procready(t->proc);

    t = p;
  }

  unlock(&b->lock);
  bindingfree(b);

  return ERR;
}

