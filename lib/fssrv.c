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
  struct response resp;
  struct request req;
  size_t len;

  while (true) {
    if (read(in, (void *) &req, sizeof(struct request)) <= 0) {
      return ELINK;
    }

    resp.head.rid = req.head.rid;
    resp.head.ret = ENOIMPL;

    len = sizeof(resp.head) - sizeof(resp.head.rid);
    
    switch (req.head.type) {
    case REQ_getfid:
      len += sizeof(struct response_getfid_b);
      if (mount->getfid)
	mount->getfid((struct request_getfid *) &req,
		      (struct response_getfid *) &resp);
      break;

    case REQ_clunk:
      len += sizeof(struct response_clunk_b);
      if (mount->clunk)
	mount->clunk((struct request_clunk *) &req,
		     (struct response_clunk*) &resp);
      break;

    case REQ_stat:
      len += sizeof(struct response_stat_b);
      if (mount->stat)
	mount->stat((struct request_stat *) &req,
		    (struct response_stat *) &resp);
      break;

    case REQ_open:
      len += sizeof(struct response_open_b);
      if (mount->open)
	mount->open((struct request_open *) &req,
		    (struct response_open *) &resp);
      break;

    case REQ_close:
      len += sizeof(struct response_close_b);
      if (mount->close)
	mount->close((struct request_close *) &req,
		     (struct response_close *) &resp);
      break;

    case REQ_create:
      len += sizeof(struct response_create_b);
      if (mount->create)
	mount->create((struct request_create *) &req,
		      (struct response_create *) &resp);
      break;

    case REQ_remove:
      len += sizeof(struct response_remove_b);
      if (mount->remove)
	mount->remove((struct request_remove *) &req,
		      (struct response_remove *) &resp);
      break;

    case REQ_read:
      resp.read.data = mount->databuf;

      if (req.read.len > mount->buflen) {
	req.read.len = mount->buflen;
      }

      if (mount->read) {
	mount->read((struct request_read *) &req,
		    (struct response_read *) &resp);
      } else {
	resp.read.len = 0;
      }
      break;

    case REQ_write:
      req.write.data = mount->databuf;

      if (req.write.len > mount->buflen) {
	req.write.len = mount->buflen;
      }
      
      if (read(in, req.write.data, req.write.len) < 0) {
	return ELINK;
      }

      len += sizeof(struct response_write_b);
      if (mount->write) {
	mount->write((struct request_write *) &req,
		     (struct response_write *) &resp);
      }
      break;
    }

    if (write(out, &resp.head.rid, sizeof(resp.head.rid))
	!= sizeof(resp.head.rid)) {
      return ELINK;
    }

    if (write(out, &resp.head.ret, len) < 0) {
      return ELINK;
    }

    if (req.head.type == REQ_read && resp.head.ret == OK) {
      if (write(out, resp.read.data, resp.read.len) < 0) {
	return ELINK;
      }
    }
  }

  return 0;
}
