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
  char name[FS_LNAME_MAX];

  struct stat stat;
  uint32_t open;

  struct file *cnext;
  struct file *parent;
  struct file *children;
};

static struct file *root = nil;
static uint32_t nfid = ROOTFID;

struct binding *rootbinding;

static int
rootproc(void *);

void
initroot(void)
{
  struct chan **c;
  struct proc *pr, *ps;

  c = malloc(sizeof(struct chan *) * 4);
  if (c == nil) {
    panic("initroot: malloc failed!\n");
  }
  
  if (!newpipe(&(c[0]), &(c[1]))) {
    panic("initroot: newpipe failed!\n");
  } else if (!newpipe(&(c[2]), &(c[3]))) {
    panic("initroot: newpipe failed!\n");
  }

  rootbinding = newbinding(nil, c[1], c[2]);
  if (rootbinding == nil) {
    panic("initroot: newbinding failed!\n");
  }

  atomicdec(&c[1]->refs);
  atomicdec(&c[2]->refs);
  
  pr = newproc();
  if (pr == nil) {
    panic("initroot: newproc failed!\n");
  }

  forkfunc(pr, &rootproc, (void *) c);

  pr->fgroup = newfgroup();
  pr->ngroup = nil;
  pr->parent = nil;
	
  ps = newproc();
  if (ps == nil) {
    panic("initroot: newproc failed!\n");
  }

  forkfunc(ps, &mountproc, (void *) rootbinding);
	
  rootbinding->srv = ps;
	
  procready(pr);
  procready(ps);
}

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

static void
bcreate(struct request *req, struct response *resp)
{
  uint32_t attr;
  struct file *p, *new;
  uint8_t *buf, lname;

  buf = req->buf;
  memmove(&attr, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(&lname, buf, sizeof(uint8_t));
  buf += sizeof(uint8_t);

  p = findfile(root, req->fid);
  
  if (p == nil || !(p->stat.attr & ATTR_dir)) {
    resp->ret = ENOFILE;
    return;
  }

  new = malloc(sizeof(struct file));
  if (new == nil) {
    resp->ret = ENOMEM;
    return;
  }

  new->stat.attr = attr;
  new->stat.size = 0;
  new->fid = nfid++;
  new->open = 0;
  new->parent = p;
  new->children = nil;

  new->lname = lname;
  memmove(new->name, buf, lname);
  new->name[lname] = 0;

  resp->buf = malloc(sizeof(uint32_t));
  if (resp->buf == nil) {
    resp->ret = ENOMEM;
    free(new);
    return;
  }

  p->stat.size += sizeof(uint8_t) + lname;
  new->cnext = p->children;
  p->children = new;
  
  resp->ret = OK;
  resp->lbuf = sizeof(uint32_t);

  memmove(resp->buf, &new->fid, sizeof(uint32_t));
}

static void
bremove(struct request *req, struct response *resp)
{
  resp->ret = ENOIMPL;
}

static void
bread(struct request *req, struct response *resp)
{
  struct file *f, *c;
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
    return;
  } else if (!(f->stat.attr & ATTR_dir)) {
    resp->ret = ENOIMPL;
    return;
  } else if (offset >= f->stat.size) {
    resp->ret = EOF;
    return;
  }

  buf = malloc(f->stat.size);
  if (buf == nil) {
    resp->ret = ENOMEM;
    return;
  }

  resp->buf = buf;
  for (c = f->children; c != nil; c = c->cnext) {
    memmove(buf, &c->lname, sizeof(uint8_t));
    buf += sizeof(uint8_t);
    memmove(buf, c->name, sizeof(uint8_t) * c->lname);
    buf += sizeof(uint8_t) * c->lname;
  }

  buf = resp->buf;
  resp->buf = buf + offset;
  resp->lbuf = len;
  resp->ret = OK;
}


static struct fsmount mount = {
  &bfid,
  &bstat,
  &bopen,
  &bclose,
  &bcreate,
  &bremove,
  &bread,
  nil,
  nil,
};

static int
rootproc(void *arg)
{
  struct chan **c;
  int in, out;

  c = (struct chan **) arg;
  
  in = addchan(current->fgroup, c[0]);
  out = addchan(current->fgroup, c[3]);

  free(c);

  root = malloc(sizeof(struct file));
  if (root == nil) {
    panic("root tree malloc failed!\n");
  }

  root->fid = nfid++;
  root->lname = 0;

  root->stat.attr = ATTR_rd|ATTR_wr|ATTR_dir;
  root->stat.size = 0;
  root->open = 0;
  root->cnext = nil;
  root->parent = nil;
  root->children = nil;

  return fsmountloop(in, out, &mount);
}
