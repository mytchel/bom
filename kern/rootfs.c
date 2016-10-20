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

  uint8_t lname;
  char name[NAMEMAX];

  uint8_t *buf;
  size_t lbuf;

  struct stat stat;
  unsigned int open;

  struct file *cnext;
  struct file *parent;
  struct file *children;
};

static struct file *root = nil;
static uint32_t nfid = ROOTFID;
static struct chan *rootin;

struct binding *rootfsbinding;

static int
rootfsproc(void *);

void
rootfsinit(void)
{
  struct chan *out;
  struct proc *pr;

  if (!pipenew(&(rootin), &(out))) {
    panic("rootfs: pipenew failed!\n");
  }

  rootfsbinding = bindingnew(out, nil, ATTR_wr|ATTR_rd|ATTR_dir);
  if (rootfsbinding == nil) {
    panic("rootfs: bindingnew failed!\n");
  }

  pr = procnew(8);
  if (pr == nil) {
    panic("rootfs: procnew failed!\n");
  }

  forkfunc(pr, &rootfsproc, nil);

  procready(pr);
}

static struct file *
bfindfile(struct file *t, uint32_t fid)
{
  struct file *c;

  if (t == nil) {
    return nil;
  } else if (t->fid == fid) {
    return t;
  } else {
    c = bfindfile(t->cnext, fid);
    if (c != nil) {
      return c;
    }

    return bfindfile(t->children, fid);
  }
}

static void
bgetfid(struct request_getfid *req, struct response_getfid *resp)
{
  struct file *f, *c;

  f = bfindfile(root, req->head.fid);
  if (f == nil) {
    resp->head.ret = ENOFILE;
    return;
  }

  for (c = f->children; c != nil; c = c->cnext) {
    if (strncmp(c->name, req->body.name, NAMEMAX)) {
      break;
    }
  }

  if (c == nil) {
    resp->head.ret = ENOCHILD;
    return;
  }

  resp->body.attr = c->stat.attr;
  resp->body.fid = c->fid;
  resp->head.ret = OK;
}

static void
bclunk(struct request_clunk *req, struct response_clunk *resp)
{
  resp->head.ret = OK;
}

static void
bstat(struct request_stat *req, struct response_stat *resp)
{
  struct file *f;

  f = bfindfile(root, req->head.fid);
  if (f == nil) {
    resp->head.ret = ENOFILE;
    return;
  }

  memmove(&resp->body.stat, &f->stat, sizeof(struct stat));
  resp->head.ret = OK;
}

static void
bopen(struct request_open *req, struct response_open *resp)
{
  struct file *f;

  f = bfindfile(root, req->head.fid);
  if (f == nil) {
    resp->head.ret = ENOFILE;
  } else {
    resp->head.ret = OK;
    f->open++;
  }
}

static void
bclose(struct request_close *req, struct response_close *resp)
{
  struct file *f;

  f = bfindfile(root, req->head.fid);
  if (f == nil) {
    resp->head.ret = ENOFILE;
  } else {
    resp->head.ret = OK;
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
    free(p->buf);
  }
  
  memmove(tbuf, &c->lname, sizeof(uint8_t));
  tbuf += sizeof(uint8_t);
  memmove(tbuf, c->name, c->lname);

  p->buf = buf;
  p->lbuf = p->lbuf + sizeof(uint8_t) + c->lname;
  
  p->stat.size = p->lbuf;

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

  if (p->lbuf - 1 - f->lname > 0) {
    buf = malloc(p->lbuf - 1 - f->lname);
    if (buf == nil) {
      return ENOMEM;
    }
  } else {
    buf = nil;
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

  free(p->buf);
  
  p->buf = buf;
  p->lbuf = p->lbuf - 1 - f->lname;
  
  p->stat.size = p->lbuf;

  free(f);
  return OK;
}

static void
bcreate(struct request_create *req, struct response_create *resp)
{
  struct file *p, *new;

  p = bfindfile(root, req->head.fid);
  
  if (p == nil || (p->stat.attr & ATTR_dir) == 0) {
    resp->head.ret = ENOFILE;
    return;
  }

  new = malloc(sizeof(struct file));
  if (new == nil) {
    resp->head.ret = ENOMEM;
    return;
  }

  new->buf = 0;
  new->lbuf = 0;
  new->stat.attr = req->body.attr;
  new->stat.size = 0;
  new->fid = nfid++;
  new->open = 0;
  new->children = nil;

  new->lname = strlcpy(new->name, req->body.name, NAMEMAX);

  resp->head.ret = addchild(p, new);
  if (resp->head.ret != OK) {
    free(new);
  } else {
    resp->body.fid = new->fid;
  }
}

static void
bremove(struct request_remove *req, struct response_remove *resp)
{
  struct file *f;

  f = bfindfile(root, req->head.fid);
  if (f == nil) {
    resp->head.ret = ENOFILE;
    return;
  } else if (f->open > 0) {
    resp->head.ret = ERR;
    return;
  } else if (f->children != nil) {
    resp->head.ret = ENOTEMPTY;
    return;
  }

  resp->head.ret = removechild(f->parent, f);
}

static void
breaddir(struct request_read *req, struct response_read *resp,
	 struct file *file,
	 uint32_t offset, uint32_t len)
{
  if (offset + len > file->lbuf) {
    len = file->lbuf - offset;
  }
  
  memmove(resp->body.data, file->buf + offset, len);
  resp->body.len = len;
  resp->head.ret = OK;
}

static void
bread(struct request_read *req, struct response_read *resp)
{
  struct file *f;

  f = bfindfile(root, req->head.fid);
  if (f == nil) {
    resp->head.ret = ENOFILE;
  } else if (req->body.offset >= f->lbuf) {
    resp->head.ret = EOF;
  } else if (f->stat.attr & ATTR_dir) {
    breaddir(req, resp, f, req->body.offset, req->body.len);
  } else {
    resp->head.ret = ENOIMPL;
  }
}

static struct fsmount rootmount = {
  .getfid = &bgetfid,
  .clunk = &bclunk,
  .stat = &bstat,
  .open = &bopen,
  .close = &bclose,
  .create = &bcreate,
  .remove = &bremove,
  .read = &bread,
};
  
static int
rootfsproc(void *arg)
{
  root = malloc(sizeof(struct file));
  if (root == nil) {
    panic("rootfs: root tree malloc failed!\n");
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

  return kmountloop(rootin, rootfsbinding, &rootmount);
}
