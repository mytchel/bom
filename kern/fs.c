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
kmountloop(struct chan *in, struct binding *b, struct fsmount *mount)
{
  struct request req;
  struct response *resp;
  struct proc *p;
  size_t reqsize;
  bool found;

  reqsize = sizeof(req.rid)
    + sizeof(req.type)
    + sizeof(req.fid)
    + sizeof(req.lbuf);

  req.buf = malloc(FS_LBUF_MAX);
  if (req.buf == nil) {
    panic("Kernel mount buf malloc failed!\n");
  }

  while (true) {
    resp = malloc(sizeof(struct response));
    if (resp == nil) {
      goto err;
    }

    if (piperead(in, (void *) &req, reqsize) != reqsize) {
      goto err;
    }

    resp->rid = req.rid;
    resp->ret = ENOIMPL;
    resp->lbuf = 0;

    if (req.lbuf > 0 &&
	piperead(in, req.buf, req.lbuf) != req.lbuf) {
      goto err;
    }

    switch (req.type) {
    case REQ_fid:
      if (mount->fid)
	mount->fid(&req, resp);
      break;
    case REQ_stat:
      if (mount->stat)
	mount->stat(&req, resp);
      break;
    case REQ_open:
      if (mount->open)
	mount->open(&req, resp);
      break;
    case REQ_close:
      if (mount->close)
	mount->close(&req, resp);
      break;
    case REQ_create:
      if (mount->create)
	mount->create(&req, resp);
      break;
    case REQ_remove:
      if (mount->remove)
	mount->remove(&req, resp);
      break;
    case REQ_read:
      if (mount->read)
	mount->read(&req, resp);
      break;
    case REQ_write:
      if (mount->write)
	mount->write(&req, resp);
      break;
    case REQ_flush:
      if (mount->flush)
	mount->flush(&req, resp);
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

    unlock(&b->lock);

    if (!found) {
      printf("no proc waiting for request %i\n", resp->rid);
      if (resp->lbuf > 0) {
	free(resp->buf);
      }

      free(resp);
    }
  }

  return 0;

 err:

  panic("Kernel mount errored!\n");

  return -1;
}


