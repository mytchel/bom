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
#include <fs.h>

bool debugfs = false;

struct dir *
walkresponsetodir(uint8_t *buf, uint32_t len)
{
  struct dir *d;
  int fi, i = 0;
	
  if (len < sizeof(uint32_t))
    return nil;

  d = (struct dir *) buf;
  i += sizeof(struct dir);
	
  if (d->nfiles == 0)
    return d;

  if (i >= len) return nil;
  d->files = (struct file **) &buf[i];
  i += sizeof(struct file *) * d->nfiles; 

  for (fi = 0; fi < d->nfiles; fi++) {
    if (i % sizeof(void *) > 0)
      i += sizeof(void *) - (i % sizeof(void *));
		
    if (i >= len) return nil;
    d->files[fi] = (struct file *) &buf[i];
    i += sizeof(struct file);

    if (i >= len) return nil;
    d->files[fi]->name = &buf[i];
    i += d->files[fi]->lname;
  }
	
  return d;
}

uint8_t *
dirtowalkresponse(struct dir *dir, uint32_t *size)
{
  uint8_t *buf;
  struct file *f;
  uint32_t i, fi;
	
  i = sizeof(struct dir);
  i += sizeof(struct file *) * dir->nfiles;
  for (fi = 0; fi < dir->nfiles; fi++) {
    i += sizeof(struct file);
    i += dir->files[fi]->lname;

    i = roundptr(i);
  }
	
  *size = i;

  buf = malloc(i);
  if (buf == nil) {
    *size = 0;
    return nil;
  }
	
  i = 0;
  memmove(&buf[i], (const void *) dir, sizeof(struct dir));
  i += sizeof(struct dir);
  i += sizeof(struct file *) * dir->nfiles;
	
  for (fi = 0; fi < dir->nfiles; fi++) {
    f = dir->files[fi];
		
    memmove(&buf[i], (const void *) f, sizeof(struct file));
    i += sizeof(struct file);
    memmove(&buf[i], (const void *) f->name, f->lname);
    i += f->lname;

    i = roundptr(i);
  }
	
  return buf;
}

int
fsmountloop(int in, int out, struct fsmount *mount)
{
  uint8_t buf[1024];/*FS_MAX_BUF_LEN];*/
  size_t reqsize, respsize;
  struct request req;
  struct response resp;
  int pid = getpid();

  reqsize = sizeof(req.rid)
    + sizeof(req.type)
    + sizeof(req.fid)
    + sizeof(req.lbuf);

  respsize = sizeof(resp.rid)
    + sizeof(resp.ret)
    + sizeof(resp.lbuf);
  
  req.buf = buf;

  if (debugfs)
    printf("fs %i start mount loop.\n", pid);

  while (true) {
    if (debugfs)
      printf("fs %i wait for request.\n", pid);

    if (read(in, &req, reqsize) != reqsize)
      goto err;

    resp.rid = req.rid;
    resp.lbuf = 0;
    resp.ret = ENOIMPL;

    if (debugfs)
      printf("fs %i got request rid = %i, lbuf = %i, type = %i\n",
	     pid, req.rid, req.lbuf, req.type);

    if (req.lbuf > 0 && read(in, req.buf, req.lbuf) != req.lbuf) {
      goto err;
    }

    if (debugfs)
      printf("fs %i process request %i\n", pid, req.rid);

    switch (req.type) {
    case REQ_open:
      if (mount->open)
	mount->open(&req, &resp);
      break;
    case REQ_close:
      if (mount->close)
	mount->close(&req, &resp);
      break;
    case REQ_walk:
      if (mount->walk)
	mount->walk(&req, &resp);
      break;
    case REQ_read:
      if (mount->read)
	mount->read(&req, &resp);
      break;
    case REQ_write:
      if (mount->write)
	mount->write(&req, &resp);
      break;
    case REQ_remove:
      if (mount->remove)
	mount->remove(&req, &resp);
      break;
    case REQ_create:
      if (mount->create)
	mount->create(&req, &resp);
      break;
    }

    if (debugfs)
      printf("fs %i hand over response for %i\n", pid, req.rid);

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

  if (debugfs)
    printf("fs %i errored.\n", pid);

  if (resp.lbuf > 0)
    free(resp.buf);

  return -1;
}
