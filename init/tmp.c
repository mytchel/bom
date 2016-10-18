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
bfid(struct request *req, struct response *resp)
{
  struct file *f, *c;
  struct response_fid *fidresp;

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

  fidresp = malloc(sizeof(struct response_fid));
  if (fidresp == nil) {
    resp->ret = ENOMEM;
    return;
  }

  fidresp->fid = c->fid;
  fidresp->attr = c->stat.attr;
  
  resp->buf = (uint8_t *) fidresp;
  resp->lbuf = sizeof(struct response_fid);
  resp->ret = OK;
}

static void
bclunk(struct request *req, struct response *resp)
{
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
    tbuf = buf + p->lbuf;
    free(p->buf);
  }
  
  memmove(tbuf, &c->lname, sizeof(uint8_t));
  tbuf += sizeof(uint8_t);
  memmove(tbuf, c->name, sizeof(uint8_t) * c->lname);

  p->buf = buf;
  p->lbuf = p->lbuf + sizeof(uint8_t) * (1 + c->lname);
  
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

  if (f->buf != nil) {
    rmmem(f->buf, f->stat.size);
  }

  free(f);
  return OK;
}

static void
bcreate(struct request *req, struct response *resp)
{
  struct request_create *cr;
  struct file *p, *new;

  cr = (struct request_create *) req->buf;

  p = findfile(root, req->fid);
  
  if (p == nil || (p->stat.attr & ATTR_dir) == 0) {
    resp->ret = ENOFILE;
    return;
  }

  new = malloc(sizeof(struct file));
  if (new == nil) {
    resp->ret = ENOMEM;
    return;
  }

  new->buf = 0;
  new->lbuf = 0;
  new->stat.attr = cr->attr;
  new->stat.size = 0;
  new->fid = nfid++;
  new->open = 0;
  new->children = nil;

  new->lname = strlcpy(new->name, cr->name, NAMEMAX);

  resp->ret = addchild(p, new);
  if (resp->ret != OK) {
    free(new);
  } else {
    resp->ret = new->fid;
  }
}

static void
bremove(struct request *req, struct response *resp)
{
  struct file *f;

  f = findfile(root, req->fid);
  if (f == nil) {
    resp->ret = ENOFILE;
    return;
  } else if (f->open > 0) {
    resp->ret = ERR;
    return;
  } else if (f->children != nil) {
    resp->ret = ENOTEMPTY;
    return;
  }

  resp->ret = removechild(f->parent, f);
}

static void
breaddir(struct request *req, struct response *resp,
	 struct file *file,
	 struct request_read *rr)
{
  uint32_t rlen;
  
  if (rr->offset + rr->len > file->lbuf) {
    rlen = file->lbuf - rr->offset;
  } else {
    rlen = rr->len;
  }
  
  resp->buf = malloc(rlen);
  if (resp->buf == nil) {
    resp->ret = ENOMEM;
    return;
  }

  memmove(resp->buf, file->buf + rr->offset, rlen);
  resp->lbuf = rlen;
  resp->ret = OK;
}

static void
breadfile(struct request *req, struct response *resp,
	 struct file *file,
	 struct request_read *rr)
{
  uint32_t rlen;

  if (rr->offset + rr->len >= file->lbuf) {
    rlen = file->lbuf - rr->offset;
  } else {
    rlen = rr->len;
  }

  resp->buf = malloc(rlen);
  if (resp->buf == nil) {
    resp->ret = ENOIMPL;
    return;
  }

  memmove(resp->buf, file->buf + rr->offset, rlen);
  
  resp->lbuf = rlen;
  resp->ret = OK;
}

static void
bread(struct request *req, struct response *resp)
{
  struct request_read *rr;
  struct file *f;

  rr = (struct request_read *) req->buf;
  
  f = findfile(root, req->fid);
  if (f == nil) {
    resp->ret = ENOFILE;
  } else if (rr->offset >= f->lbuf) {
    resp->ret = EOF;
  } else if (f->stat.attr & ATTR_dir) {
    breaddir(req, resp, f, rr);
  } else {
    breadfile(req, resp, f, rr);
  }
}

static void
bwrite(struct request *req, struct response *resp)
{
  struct request_write *wr;
  struct file *f;
  uint8_t *nbuf;
  size_t nsize;

  wr = (struct request_write *) req->buf;

  printf("tmpfs bwrite from %i len %i\n", wr->offset,
	 wr->len);

  f = findfile(root, req->fid);
  if (f == nil) {
    resp->ret = ENOFILE;
    return;
  }

  if (wr->offset + wr->len > f->stat.size) {
    nsize = wr->offset + wr->len;
    nbuf = getmem(MEM_ram, nil, &nsize);
    if (nbuf == nil) {
      resp->ret = ENOMEM;
      return;
    }

    if (f->stat.size > 0) {
      memmove(nbuf, f->buf, f->stat.size);
      rmmem(f->buf, f->stat.size);
    }

    f->stat.size = nsize;
    f->buf = nbuf;
  }

  if (wr->offset + wr->len > f->lbuf) {
    f->lbuf = wr->offset + wr->len;
  }

  memmove(f->buf + wr->offset, wr->buf, wr->len);

  resp->ret = wr->len;
}
 
static struct fsmount mount = {
  .fid = &bfid,
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
  int f, fd, p1[2], p2[2];
  
  if (pipe(p1) == ERR) {
    return ERR;
  } else if (pipe(p2) == ERR) {
    return ERR;
  }

  fd = open(path, O_WRONLY|O_CREATE, ATTR_wr|ATTR_rd|ATTR_dir);
  if (fd < 0) {
    return ERR;
  }

  if (bind(p1[1], p2[0], path) == ERR) {
    return ERR;
  }
	
  close(p1[1]);
  close(p2[0]);

  f = fork(FORK_sngroup);
  if (f != 0) {
    close(p1[0]);
    close(p2[1]);
    close(fd);
    return f;
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
  root->open = 0;
  root->cnext = nil;
  root->parent = nil;
  root->children = nil;

  f = fsmountloop(p1[0], p2[1], &mount);
  exit(f);
}
