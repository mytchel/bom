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

static bool
respread(struct chan *c, struct response *resp)
{
  size_t size;

  size = sizeof(resp->rid)
    + sizeof(resp->ret)
    + sizeof(resp->lbuf);
  
  if (piperead(c, (void *) resp, size) != size) {
    return false;
  }

  if (resp->lbuf > 0) {
    resp->buf = malloc(resp->lbuf);
    if (resp->buf == nil) {
      printf("kproc mount: error allocating resp buf\n");
      return false;
    }

    if (piperead(c, resp->buf, resp->lbuf) != resp->lbuf) {
      printf("kproc mount: Error reading resp buf\n");
      return false;
    }
  }
	
  return true;
}

int
mountproc(void *arg)
{
  struct binding *b;
  struct proc *p, *pn;
  struct response *resp;
  bool found;

  b = (struct binding *) arg;

  while (true) {
    resp = malloc(sizeof(struct response));
    if (resp == nil) {
      printf("kproc mount: error allocating response.\n");
      break;
    }

    if (!respread(b->in, resp)) {
      printf("kproc mount: error reading responses.\n");
      break;
    }

    lock(&b->lock);

    found = false;
    for (p = b->waiting; p != nil; p = p->next) {
      if ((uint32_t) p->aux == resp->rid) {
	found = true;
	p->aux = (void *) resp;

	disableintr();
	procready(p);
	enableintr();
	break;
      }
    }

    if (!found) {
      if (resp->lbuf > 0)
	free(resp->buf);
      free(resp);
    }
    
    unlock(&b->lock);
  }

  lock(&b->lock);

  chanfree(b->in);
  chanfree(b->out);

  b->in = b->out = nil;

  /* Wake up any waiting processes so they can error. */

  p = b->waiting;
  while (p != nil) {
    pn = p->next;

    p->aux = nil;
    disableintr();
    procready(p);
    enableintr();

    p = pn;
  }

  /* Free binding and exit. */

  unlock(&b->lock);
  bindingfree(b);

  disableintr();

  procexit(up, ERR);
  schedule();

  /* Never reached */
  return 0;
}
