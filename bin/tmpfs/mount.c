/*
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
#include <stdarg.h>
#include <string.h>

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
bgetfid(struct request_getfid *req, struct response_getfid *resp)
{
  struct file *f, *c;

  f = findfile(root, req->head.fid);
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

  resp->body.fid = c->fid;
  resp->body.attr = c->stat.attr;
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

  f = findfile(root, req->head.fid);
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

  f = findfile(root, req->head.fid);
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

  f = findfile(root, req->head.fid);
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
  
  buf = malloc(p->lbuf + 1 + c->lname);
  if (buf == nil) {
    return ENOMEM;
  }

  tbuf = buf;
  if (p->lbuf > 0) {
    memmove(tbuf, p->buf, p->lbuf);
    tbuf = buf + p->lbuf;
    free(p->buf);
  }
  
  memmove(tbuf, &c->lname, sizeof(uint8_t));
  tbuf += sizeof(uint8_t);
  memmove(tbuf, c->name, c->lname);

  p->buf = buf;
  p->lbuf += 1 + c->lname;
  p->stat.dsize = sizeof(struct file) + p->lbuf;
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

  if (f->lbuf > 0) {
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
  p->lbuf -= 1 + f->lname;
  p->stat.dsize = sizeof(struct file) + p->lbuf;
  p->stat.size--;

  if (f->buf != nil) {
    munmap(f->buf, f->stat.dsize);
  }

  free(f);
  return OK;
}

static void
bcreate(struct request_create *req, struct response_create *resp)
{
  struct file *p, *new;

  p = findfile(root, req->head.fid);
  
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
  new->stat.dsize = sizeof(struct file);
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

  f = findfile(root, req->head.fid);
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
	 struct file *f, uint32_t offset, uint32_t len)
{
  if (offset >= f->lbuf) {
    len = 0;
  } else if (offset + len > f->lbuf) {
    len = f->lbuf - offset;
  }

  if (len > 0) {
    memmove(resp->body.data, f->buf + offset, len);
  }

  resp->body.len = len;
  resp->head.ret = OK;
}

static void
breadfile(struct request_read *req, struct response_read *resp,
	 struct file *f, uint32_t offset, uint32_t len)
{
  if (offset >= f->stat.size) {
    len = 0;
  } else if (offset + len > f->stat.size) {
    len = f->stat.size - offset;
  }

  if (len > 0) {
    memmove(resp->body.data, f->buf + offset, len);
  }

  resp->body.len = len;
  resp->head.ret = OK;
}

static void
bread(struct request_read *req, struct response_read *resp)
{
  struct file *f;
  
  f = findfile(root, req->head.fid);
  if (f == nil) {
    resp->head.ret = ENOFILE;
  } else if (f->stat.attr & ATTR_dir) {
    breaddir(req, resp, f, req->body.offset, req->body.len);
  } else {
    breadfile(req, resp, f, req->body.offset, req->body.len);
  }
}

static void
bwrite(struct request_write *req, struct response_write *resp)
{
  struct file *f;
  uint8_t *nbuf;
  size_t nsize;

  f = findfile(root, req->head.fid);
  if (f == nil) {
    resp->head.ret = ENOFILE;
    return;
  }

  if (req->body.offset + req->body.len > f->lbuf) {
    nsize = req->body.offset + req->body.len;
    nbuf = mmap(MEM_ram, nil, &nsize);
    if (nbuf == nil) {
      resp->head.ret = ENOMEM;
      return;
    }

    if (f->lbuf > 0) {
      memmove(nbuf, f->buf, f->stat.size);
      munmap(f->buf, f->lbuf);
    }

    f->buf = nbuf;
    f->lbuf = nsize;
    f->stat.dsize = sizeof(struct file) + nsize;
  }

  if (req->body.offset + req->body.len > f->stat.size) {
    f->stat.size = req->body.offset + req->body.len;
  }

  memmove(f->buf + req->body.offset, req->body.data, req->body.len);
  resp->body.len = req->body.len;
  resp->head.ret = OK;
}
 
static struct fsmount fsmount = {
  .getfid = &bgetfid,
  .clunk = &bclunk,
  .stat = &bstat,
  .open = &bopen,
  .close = &bclose,
  .create = &bcreate,
  .remove = &bremove,
  .read = &bread,
  .write = &bwrite,
};

int
mounttmp(char *path)
{
  int f, p1[2], p2[2];
  
  if (pipe(p1) == ERR) {
    return ERR;
  } else if (pipe(p2) == ERR) {
    return ERR;
  }

  if (mount(p1[1], p2[0], path, ATTR_rd|ATTR_wr|ATTR_dir) == ERR) {
    return ERR;
  }
	
  close(p1[1]);
  close(p2[0]);

  f = fork(FORK_sngroup);
  if (f != 0) {
    close(p1[0]);
    close(p2[1]);
    return OK;
  }

  root = malloc(sizeof(struct file));
  if (root == nil) {
    printf("tmp root tree malloc failed!\n");
    exit(ENOMEM);
  }

  root->fid = nfid++;
  root->lname = 0;

  root->lbuf = 0;
  root->buf = nil;
  
  root->stat.attr = ATTR_rd|ATTR_wr|ATTR_dir;
  root->stat.size = 0;
  root->stat.dsize = sizeof(struct file);
  root->open = 0;
  root->cnext = nil;
  root->parent = nil;
  root->children = nil;

  fsmount.databuf = malloc(512);
  fsmount.buflen = 512;

  f = fsmountloop(p1[0], p2[1], &fsmount);

  printf("tmp mount on %s exiting with %i\n",
	 path, f);
  
  free(fsmount.databuf);
  
  exit(f);
}
