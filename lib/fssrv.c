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

#include <libc.h>
#include <mem.h>
#include <fs.h>
#include <fssrv.h>

int
fsmountloop(int in, int out, struct fsmount *mount)
{
  uint8_t buf[FSBUFMAX];
  size_t reqsize, respsize;
  struct response resp;
  struct request req;

  reqsize = sizeof(req.rid)
    + sizeof(req.type)
    + sizeof(req.fid)
    + sizeof(req.lbuf);

  respsize = sizeof(resp.rid)
    + sizeof(resp.ret)
    + sizeof(resp.lbuf);
  
  req.buf = buf;

  while (true) {
    if (read(in, &req, reqsize) != reqsize)
      goto err;

    resp.rid = req.rid;
    resp.lbuf = 0;
    resp.ret = ENOIMPL;

    if (req.lbuf > 0 && read(in, req.buf, req.lbuf) != req.lbuf) {
      goto err;
    }

    switch (req.type) {
    case REQ_fid:
      if (mount->fid)
	mount->fid(&req, &resp);
      break;
    case REQ_clunk:
      if (mount->clunk)
	mount->clunk(&req, &resp);
      break;
    case REQ_stat:
      if (mount->stat)
	mount->stat(&req, &resp);
      break;
    case REQ_open:
      if (mount->open)
	mount->open(&req, &resp);
      break;
    case REQ_close:
      if (mount->close)
	mount->close(&req, &resp);
      break;
    case REQ_create:
      if (mount->create)
	mount->create(&req, &resp);
      break;
    case REQ_remove:
      if (mount->remove)
	mount->remove(&req, &resp);
      break;
    case REQ_read:
      if (mount->read)
	mount->read(&req, &resp);
      break;
    case REQ_write:
      if (mount->write)
	mount->write(&req, &resp);
      break;
    }

    if (write(out, &resp, respsize) != respsize)
      goto err;
		
    if (resp.lbuf > 0) {
      if (write(out, resp.buf, resp.lbuf) != resp.lbuf) {
	goto err;
      }

      free(resp.buf);
    }
  }

  return 0;

 err:

  if (resp.lbuf > 0) {
    free(resp.buf);
  }

  return -1;
}
