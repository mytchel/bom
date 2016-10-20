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
  struct response badresp, *resp;
  struct binding *b;
  struct proc *p, *pn;
  uint32_t rid;
  
  b = (struct binding *) arg;

  while (true) {
    if (piperead(b->in, (void *) &rid, sizeof(rid)) != sizeof(rid)) {
      printf("kproc mount: error reading rid.\n");
      break;
    }

    lock(&b->lock);

    resp = &badresp;
    for (p = b->waiting; p != nil; p = p->next) {
      resp = (struct response *) p->aux;
      if (resp->head.rid == rid) {
	break;
      }
    }

    unlock(&b->lock);

    if (piperead(b->in, (void *) &resp->head.ret,
		 sizeof(struct response) - sizeof(rid)) < 0) {
      printf("kproc mount: error reading response.\n");
      break;
    }

    if (p != nil) {
      setintr(INTR_OFF);
      procready(p);
      setintr(INTR_ON);
    }
  }

  lock(&b->lock);

  chanfree(b->in);
  chanfree(b->out);

  /* Wake up any waiting processes so they can error. */

  setintr(INTR_OFF);

  p = b->waiting;
  while (p != nil) {
    pn = p->next;

    ((struct response *) p->aux)->head.ret = ELINK;
    procready(p);

    p = pn;
  }

  /* Free binding and exit. */

  unlock(&b->lock);
  bindingfree(b);

  setintr(INTR_OFF);

  procexit(up, ERR);
  schedule();

  /* Never reached */
  return 0;
}

int
kmountloop(struct chan *in, struct binding *b, struct fsmount *mount)
{
  struct response *resp;
  struct request req;
  struct proc *p;

  while (true) {
    if (piperead(in, (void *) &req, sizeof(req)) <= 0) {
      return ELINK;
    }

    lock(&b->lock);

    for (p = b->waiting; p != nil; p = p->next) {
      if (((struct response *) p->aux)->head.rid == req.head.rid) {
	break;
      }
    }

    unlock(&b->lock);

    if (p == nil) {
      printf("kmountloop: request %i doesnt have proc!\n",
	     req.head.rid);
      continue;
    }

    resp = (struct response *) p->aux;
    resp->head.ret = ENOIMPL;

    switch (req.head.type) {
    case REQ_getfid:
      if (mount->getfid)
	mount->getfid((struct request_getfid *) &req,
		      (struct response_getfid *) resp);
      break;
    case REQ_clunk:
      if (mount->clunk)
	mount->clunk((struct request_clunk *) &req,
		     (struct response_clunk*) resp);
      break;
    case REQ_stat:
      if (mount->stat)
	mount->stat((struct request_stat *) &req,
		    (struct response_stat *) resp);
      break;
    case REQ_open:
      if (mount->open)
	mount->open((struct request_open *) &req,
		    (struct response_open *) resp);
      break;
    case REQ_close:
      if (mount->close)
	mount->close((struct request_close *) &req,
		     (struct response_close *) resp);
      break;
    case REQ_create:
      if (mount->create)
	mount->create((struct request_create *) &req,
		      (struct response_create *) resp);
      break;
    case REQ_remove:
      if (mount->remove)
	mount->remove((struct request_remove *) &req,
		      (struct response_remove *) resp);
      break;
    case REQ_read:
      if (mount->read)
	mount->read((struct request_read *) &req,
		    (struct response_read *) resp);
      break;
    case REQ_write:
      if (mount->write)
	mount->write((struct request_write *) &req,
		     (struct response_write *) resp);
      break;
    }

    setintr(INTR_OFF);
    procready(p);
    setintr(INTR_ON);
  }

  return 0;
}
