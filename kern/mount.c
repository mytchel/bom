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
  struct fstransaction *trans;
  struct response *resp;
  struct binding *b;
  struct proc *p, *pn;
  uint32_t rid, rlen = 0;
  
  b = (struct binding *) arg;

  while (true) {
    if (piperead(b->in, (void *) &rid, sizeof(rid)) != sizeof(rid)) {
      printf("kproc mount: error reading rid.\n");
      break;
    }

    lock(&b->lock);

    resp = nil;
    for (p = b->waiting; p != nil; p = p->next) {
      trans = (struct fstransaction *) p->aux;
      if (trans->req->head.rid == rid) {
	resp = trans->resp;
	break;
      }
    }

    if (resp == nil) {
      printf("kproc mount: response has no waiter.\n");
      break;
    } else if (trans->req->head.type == REQ_read) {
      rlen = resp->read.len;
    }
    
    unlock(&b->lock);

    if (piperead(b->in, (void *) &resp->head.ret,
		 sizeof(struct response) - sizeof(rid)) < 0) {
      printf("kproc mount: error reading response.\n");
      break;
    }

    if (trans->req->head.type == REQ_read && resp->head.ret == OK) {
      resp->read.len = piperead(b->in, resp->read.data, rlen);
      if (resp->read.len < 0) {
	printf("kproc mount: error reading response.\n");
	break;
      }
    }

    if (p != nil) {
      setintr(INTR_OFF);
      procready(p);
      setintr(INTR_ON);
    }
  }

  lock(&b->lock);

  chanfree(b->in);
  b->in = nil;
  chanfree(b->out);
  b->out = nil;

  /* Wake up any waiting processes so they can error. */

  setintr(INTR_OFF);

  p = b->waiting;
  while (p != nil) {
    pn = p->next;
    
    trans = (struct fstransaction *) p->aux;
    trans->resp->head.ret = ELINK;
    procready(p);

    p = pn;
  }

  unlock(&b->lock);
  bindingfree(b);

  procexit(up, ERR);
  schedule();

  /* Never reached */
  return 0;
}

