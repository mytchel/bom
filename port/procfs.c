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

struct file {
  uint32_t fid;
  struct stat stat;

  struct file *cnext;
  struct file *children;
};

static struct chan *fsin;

struct binding *procfsbinding;

void
initprocfs(void)
{
  struct chan *out;

  if (!newpipe(&(fsin), &(out))) {
    panic("procfs: newpipe failed!\n");
  }

  procfsbinding = newbinding(out, nil);
  if (procfsbinding == nil) {
    panic("procfs: newbinding failed!\n");
  }

  /*
  pr = newproc(60);
  if (pr == nil) {
    panic("procfs: newproc failed!\n");
  }

  forkfunc(pr, &procfsproc, nil);
  procready(pr);
  */
}

#if 0
static struct file *
findfile(struct file *t, uint32_t fid)
{
  struct file *c;

  if (t == nil) {
    return nil;
  } else if (t->fid == fid) {
    return t;
  } else {
    c = findfile(t->cnext, fid);
    if (c != nil) {
      return c;
    }

    return findfile(t->children, fid);
  }
}

static void
bfid(struct request *req, struct response *resp)
{
  struct file *f, *c;

  f = findfile(root, req->fid);
  if (f == nil) {
    resp->ret = ENOFILE;
    return;
  }

  for (c = f->children; c != nil; c = c->cnext) {
    if (c->lname != req->lbuf) continue;
    if (strncmp(c->name, (char *) req->buf, c->lname)) {
      break;
    }
  }

  if (c == nil) {
    resp->ret = ENOCHILD;
    return;
  }

  resp->buf = malloc(sizeof(uint32_t));
  if (resp->buf == nil) {
    resp->ret = ENOMEM;
    return;
  }
  
  resp->lbuf = sizeof(uint32_t);
  memmove(resp->buf, &c->fid, sizeof(uint32_t));
  resp->ret = OK;
}


static void
bstat(struct request *req, struct response *resp)
{
  struct file *f;

  f = findfile(root, req->fid);
  if (f == nil) {
    resp->ret = ENOFILE;
    return;
  }

  resp->buf = malloc(sizeof(struct stat));
  if (resp->buf == nil) {
    resp->ret = ENOMEM;
  } else {
    resp->ret = OK;
    resp->lbuf = sizeof(struct stat);
    memmove(resp->buf, &f->stat, sizeof(struct stat));
  }
}

static void
bopen(struct request *req, struct response *resp)
{
  struct file *f;

  f = findfile(root, req->fid);
  if (f == nil) {
    resp->ret = ENOFILE;
  } else {
    resp->ret = OK;
    f->open++;
  }
}

static void
bclose(struct request *req, struct response *resp)
{
  struct file *f;

  f = findfile(root, req->fid);
  if (f == nil) {
    resp->ret = ENOFILE;
  } else {
    resp->ret = OK;
    f->open--;
  }
}

static int
addchild(struct file *p, struct file *c)
{
  uint8_t *buf, *tbuf;

  buf = malloc(p->lbuf + sizeof(uint8_t) + c->lname);
  if (buf == nil) {
    return ENOMEM;
  }

  tbuf = buf;
  if (p->lbuf > 0) {
    memmove(tbuf, p->buf, p->lbuf);
    tbuf += p->lbuf;
  }
  
  memmove(tbuf, &c->lname, sizeof(uint8_t));
  tbuf += sizeof(uint8_t);
  memmove(tbuf, c->name, sizeof(uint8_t) * c->lname);

  if (p->lbuf > 0) {
    free(p->buf);
  }

  p->buf = buf;
  p->lbuf = p->lbuf + sizeof(uint8_t) + c->lname;
  
  p->stat.size++;

  c->cnext = p->children;
  p->children = c;

  c->parent = p;

  return OK;
}

static int
removechild(struct file *p, struct file *f)
{
  struct file *c;
  uint8_t *buf, *tbuf;

  if ((f->stat.attr & ATTR_dir) && f->lbuf > 0) {
    return ENOTEMPTY;
  }

  buf = malloc(p->lbuf - 1 - f->lname);
  if (buf == nil) {
    return ENOMEM;
  }

  /* Remove from parent */
  for (c = p->children; c != nil && c->cnext != f; c = c->cnext)
    ;

  if (c == nil) {
    p->children = f->cnext;
  } else {
    c->cnext = f->cnext;
  }

  tbuf = buf;
  for (c = p->children; c != nil; c = c->cnext) {
    memmove(tbuf, &c->lname, sizeof(uint8_t));
    tbuf += sizeof(uint8_t);
    memmove(tbuf, c->name, c->lname);
    tbuf += c->lname;
  }

  p->buf = buf;
  p->lbuf = p->lbuf - 1 - f->lname;
  
  p->stat.size--;

  free(f);
  return OK;
}

static void
breaddir(struct request *req, struct response *resp,
	 struct file *file,
	 uint32_t offset, uint32_t len)
{
  if (offset + len > file->lbuf) {
    len = file->lbuf - offset;
  }
  
  resp->buf = malloc(len);
  if (resp->buf == nil) {
    resp->ret = ENOMEM;
    return;
  }

  memmove(resp->buf, file->buf + offset, len);
  resp->lbuf = len;
  resp->ret = OK;
}

static void
bread(struct request *req, struct response *resp)
{
  struct file *f;
  uint32_t offset, len;
  uint8_t *buf;

  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(&len, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);

  f = findfile(root, req->fid);
  if (f == nil) {
    resp->ret = ENOFILE;
  } else if (offset >= f->lbuf) {
    resp->ret = EOF;
  } else if (f->stat.attr & ATTR_dir) {
    breaddir(req, resp, f, offset, len);
  } else {
    resp->ret = ENOIMPL;
  }
}

static struct fsmount rootmount = {
  bfid,
  bstat,
  bopen,
  bclose,
  nil,
  nil,
  bread,
  bwrite,
  nil
};

static int
procfsproc(void *arg)
{
  root = malloc(sizeof(struct file));
  if (root == nil) {
    panic("root tree malloc failed!\n");
  }

  root->fid = nfid++;
  root->lname = 0;

  root->lbuf = 0;
  root->buf = nil;
  
  root->stat.attr = ATTR_rd|ATTR_wr|ATTR_dir;
  root->stat.size = 0;
  root->open = 0;
  root->cnext = nil;
  root->parent = nil;
  root->children = nil;

  return kmountloop(fsin, fsbinding, &rootmount);
}
#endif
