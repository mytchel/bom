/*
 *   Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
    i += sizeof(uint8_t) * d->files[fi]->lname;
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
    if (i % sizeof(void *) > 0)
      i += sizeof(void *) - (i % sizeof(void *));
		
    i += sizeof(struct file);
    i += sizeof(uint8_t) * dir->files[fi]->lname;
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
    if (i % sizeof(void *) > 0)
      i += sizeof(void *) - (i % sizeof(void *));
		
    f = dir->files[fi];
		
    memmove(&buf[i], (const void *) f, sizeof(struct file));
    i += sizeof(struct file);
    memmove(&buf[i], (const void *) f->name, sizeof(uint8_t) * f->lname);
    i += sizeof(uint8_t) * f->lname;
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

  reqsize = sizeof(req.rid)
    + sizeof(req.type)
    + sizeof(req.fid)
    + sizeof(req.lbuf);

  respsize = sizeof(resp.rid)
    + sizeof(resp.ret)
    + sizeof(resp.lbuf);
  
  req.buf = buf;

  if (debugfs)
    printf("fs start mount loop.\n");

  while (true) {
    if (debugfs)
      printf("fs wait for request.\n");

    if (read(in, &req, reqsize) != reqsize)
      goto err;

    resp.rid = req.rid;
    resp.lbuf = 0;
    resp.ret = ENOIMPL;

    if (debugfs)
      printf("fs got request rid = %i, lbuf = %i, type = %i\n",
	     req.rid, req.lbuf, req.type);

    if (req.lbuf > 0 && read(in, req.buf, req.lbuf) != req.lbuf) {
      goto err;
    }

    if (debugfs)
      printf("fs process request %i\n", req.rid);

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
      printf("fs hand over response for %i\n", req.rid);

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

  if (resp.lbuf > 0)
    free(resp.buf);

  free(buf);

  return -1;
}
