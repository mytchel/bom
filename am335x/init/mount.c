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

size_t putc(uint8_t *, size_t);
size_t getc(uint8_t *, size_t);
bool uartinit(void);

struct dir_list {
  uint32_t fid;
  struct dir dir;
  struct dir_list *next;
};

struct file_list {
  struct dir *parent;
  int opened;
  struct file file;
  size_t len;
  uint8_t *buf;
  struct file_list *next;
};

struct dir_list rootdir = {
  ROOTFID,
  { 0, nil },
  nil
};

struct file_list rootfile = {
  nil,
  0,
  {
    ROOTFID,
    ATTR_rd|ATTR_dir,
    1,
    (uint8_t *) ""
  },
  0,
  nil,
  nil
};

static int nfid = 1;
static struct dir_list *dir_list = &rootdir;
static struct file_list *file_list = &rootfile;

static uint32_t
tmpread(struct file_list *fl, uint8_t *buf, uint32_t offset,
	uint32_t len, int32_t *err)
{
  uint8_t *bbuf;
  uint32_t i;

  if (offset >= fl->len) {
    *err = EOF;
    return 0;
  }
	
  bbuf = fl->buf + offset;
  for (i = 0; i + offset < fl->len && i < len; i++)
    buf[i] = bbuf[i];

  *err = OK;
  return i;
}

static uint32_t
tmpwrite(struct file_list *fl, uint8_t *buf, uint32_t offset,
	 uint32_t len, int32_t *err)
{
  uint8_t *bbuf;
  uint32_t i;
	
  if (offset + len >= fl->len) {
    bbuf = malloc(sizeof(uint8_t) * (offset + len));
    if (bbuf == nil) {
      *err = ENOMEM;
      return 0;
    }

    if (fl->len > 0) {
      memmove(bbuf, fl->buf, fl->len);		
      free(fl->buf);
    }
			
    fl->buf = bbuf;
    fl->len = offset + len;
  }

  bbuf = fl->buf + offset;
  for (i = 0; i + offset < fl->len && i < len; i++)
    bbuf[i + offset] = buf[i];
	
  *err = OK;
  return i;
}

static struct file_list *
newfile(uint32_t attr, uint8_t *name)
{
  struct file_list *fl;
	
  for (fl = file_list; fl != nil && fl->next != nil; fl = fl->next);
  if (fl == nil) {
    fl = file_list = malloc(sizeof(struct file_list));
  } else {
    fl->next = malloc(sizeof(struct file_list));
    fl = fl->next;
  }
	
  fl->next = nil;
  fl->opened = 0;
  fl->file.fid = nfid++;
  fl->file.attr = attr;
  fl->file.lname = strlen(name) + 1;
  fl->file.name = malloc(fl->file.lname);
  memmove(fl->file.name, name, fl->file.lname);

  fl->len = 0;
  fl->buf = nil;
		
  return fl;
}

static struct file_list *
findfile(uint32_t fid)
{
  struct file_list *fl;
  for (fl = file_list; fl != nil; fl = fl->next)
    if (fl->file.fid == fid)
      break;
	
  return fl;
}

static void
diraddfile(struct dir *d, struct file *f)
{
  struct file **files;
  int i;
	
  files = malloc(sizeof(struct file*) * (d->nfiles + 1));
	
  for (i = 0; i < d->nfiles; i++)
    files[i] = d->files[i];
	
  files[i] = f;
	
  if (d->nfiles > 0)
    free(d->files);
	
  d->nfiles++;
  d->files = files;
}

static void
dirremovefile(struct dir *d, struct file *f)
{
  struct file **files;
  int i, j;
	
  files = malloc(sizeof(struct file*) * (d->nfiles - 1));
	
  i = j = 0;
  while (i < d->nfiles - 1) {
    if (d->files[i]->fid != f->fid) {
      files[j++] = d->files[i];
    }
    i++;
  }
	
  d->nfiles--;
  d->files = files;
}

static struct dir *
finddir(uint32_t fid)
{
  struct dir_list *dl;
	
  for (dl = dir_list; dl != nil; dl = dl->next) {
    if (dl->fid == fid)
      return &dl->dir;
  }
  return nil;
}

static struct dir *
adddir(uint32_t fid)
{
  struct dir_list *dl;
	
  for (dl = dir_list; dl != nil && dl->next != nil; dl = dl->next);
	
  dl->next = malloc(sizeof(struct dir_list));
  dl = dl->next;
	
  if (dl == nil) {
    return nil;
  }

  dl->next = nil;
  dl->fid = fid;
  dl->dir.nfiles = 0;
  dl->dir.files = nil;
  return &dl->dir;
}

static void
bwalk(struct request *req, struct response *resp)
{
  struct dir *d;
	
  d = finddir(req->fid);
  if (d == nil) {
    resp->ret = ENOFILE;
  } else {
    resp->ret = OK;
    resp->buf = dirtowalkresponse(d, &resp->lbuf);
  }
}

static void
bopen(struct request *req, struct response *resp)
{
  struct file_list *fl;
	
  fl = findfile(req->fid);
  if (fl == nil) {
    resp->ret = ENOFILE;
    return;
  }

  fl->opened++;
  resp->ret = OK;
}

static void
bclose(struct request *req, struct response *resp)
{
  struct file_list *fl;
	
  fl = findfile(req->fid);
  if (fl == nil) {
    resp->ret = ENOFILE;
    return;
  }

  fl->opened--;
  resp->ret = OK;
}

static void
bread(struct request *req, struct response *resp)
{
  uint32_t offset, len;
  struct file_list *fl;
  uint8_t *buf;
	
  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(&len, buf, sizeof(uint32_t));

  fl = findfile(req->fid);
  if (fl == nil) {
    resp->ret = ENOFILE;
    return;
  }

  resp->ret = OK;
  resp->buf = malloc(sizeof(uint8_t) * len);
  resp->lbuf = tmpread(fl, resp->buf, offset, len, &resp->ret);
  if (resp->lbuf == 0)
    free(resp->buf);
}

static void
bwrite(struct request *req, struct response *resp)
{
  uint32_t offset, len, n;
  struct file_list *fl;
  uint8_t *buf;
	
  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(&len, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);

  fl = findfile(req->fid);
  if (fl == nil) {
    resp->ret = ENOFILE;
    return;
  }

  n = tmpwrite(fl, buf, offset, len, &resp->ret);
	
  resp->ret = OK;
  resp->lbuf = sizeof(uint32_t);
  resp->buf = malloc(sizeof(uint32_t));
  memmove(resp->buf, &n, sizeof(uint32_t));
}

static void
bcreate(struct request *req, struct response *resp)
{
  uint32_t attr, lname;
  struct file_list *fl;
  struct dir *d;
  uint8_t *buf;
	
  buf = req->buf;
  memmove(&attr, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(&lname, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
	
  d = finddir(req->fid);
  if (d == nil) {
    resp->ret = ENOFILE;
    return;
  }
	
  fl = newfile(attr, buf);

  fl->parent = d;
  diraddfile(d, &fl->file);
	
  if ((attr & ATTR_dir) == ATTR_dir) {
    adddir(fl->file.fid);
  }

  resp->ret = OK;
  resp->lbuf = sizeof(uint32_t);
  resp->buf = malloc(sizeof(uint32_t));
  memmove(resp->buf, &fl->file.fid, sizeof(uint32_t));
}

static void
bremove(struct request *req, struct response *resp)
{
  struct file_list *fl, *fp;
  struct dir_list *d, *dt;
	
  fl = findfile(req->fid);
  if (fl == nil) {
    resp->ret = ENOFILE;
    return;
  }

  if (fl->opened > 0) {
    resp->ret = ERR;
    return;
  }

  for (fp = file_list; fp != nil && fp->next != fl; fp = fp->next);
  if (fp != nil) {
    fp->next = fl->next;
  } else {
    file_list = file_list->next;
  }

  for (d = dir_list; 
       d != nil && d->next != nil && d->next->fid != req->fid; 
       d = d->next);

  if (d != nil && d->next != nil) {
    dt = d->next;
    if (dt->dir.nfiles > 0) {
      resp->ret = ERR;
      return;
    }
    free(dt);
	
    d->next = d->next->next;
  }
		
  dirremovefile(fl->parent, &fl->file);

  free(fl->file.name);
  if (fl->len > 0)
    free(fl->buf);
  free(fl);
	
  resp->ret = OK;
}

struct fsmount fsmount = {
  bopen,
  bclose,
  bwalk,
  bread,
  bwrite,
  bremove,
  bcreate,
};
