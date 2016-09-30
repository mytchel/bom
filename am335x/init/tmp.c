/*
 *
 * Copyright (c) <2016> Mytchel Hammond <mytchel@openmailbox.org>
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
#include <stdarg.h>
#include <string.h>

struct file_list {
  struct dir *parent;
  int opened;
  struct file file;
  size_t len;
  uint8_t *buf;
  struct file_list *children;
  struct file_list *next;
  struct file_list *cnext;
};

struct file_list rootfile = {
  nil,
  0,
  {
    ROOTFID,
    ATTR_rd|ATTR_dir,
    0,
    0,
    ""
  },
  0,
  nil,
  nil,
  nil,
  nil
};

static int nfid = 1;
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
    bbuf = malloc(offset + len);
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
newfile(uint32_t attr, char *name)
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
  resp->buf = malloc(len);
  resp->lbuf = tmpread(fl, resp->buf, offset, len, &resp->ret);
  if (resp->lbuf == 0)
    free(resp->buf);
}

static void
bwrite(struct request *req, struct response *resp)
{
  uint32_t offset, n;
  struct file_list *fl;
  uint8_t *buf;
	
  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);

  fl = findfile(req->fid);
  if (fl == nil) {
    resp->ret = ENOFILE;
    return;
  }

  n = tmpwrite(fl, buf, offset,
	       req->lbuf - sizeof(uint32_t), &resp->ret);

  if (resp->ret == OK) {
    resp->lbuf = sizeof(uint32_t);
    resp->buf = malloc(sizeof(uint32_t));
    memmove(resp->buf, &n, sizeof(uint32_t));
  } else {
      resp->lbuf = 0;
  }
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
	
  fl = newfile(attr, (char *) buf);

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

int
tmpmount(char *path)
{
  int f, fd, p1[2], p2[2];
  
  if (pipe(p1) == ERR) {
    return -4;
  } else if (pipe(p2) == ERR) {
    return -5;
  }

  fd = open(path, O_WRONLY|O_CREATE, ATTR_wr|ATTR_rd|ATTR_dir);
  if (fd < 0) {
    return -3;
  }

  if (bind(p1[1], p2[0], path) == ERR) {
    return -6;
  }
	
  close(p1[1]);
  close(p2[0]);

  f = fork(FORK_sngroup);
  if (f < 0) {
    return -1;
  } else if (!f) {
    return fsmountloop(p1[0], p2[1], &fsmount);
  }
	
  close(p1[0]);
  close(p2[1]);
  close(fd);
  
  return f;
}
