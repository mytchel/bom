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
  uint8_t lname;
  char name[NAMEMAX];

  struct stat stat;
  unsigned int open;

  struct file *cnext;
  struct file *parent;
  struct file *children;
};

#define FIDSMAX 512

static struct file *fids[FIDSMAX] = { nil };
static struct file *root = nil;

static int in, out;
struct binding *rootfsbinding;

static int
rootfsproc(void *);

void
rootfsinit(void)
{
  struct chan *c1[2], *c2[2];
  struct proc *pr, *pm;

  if (!pipenew(&(c1[0]), &(c1[1]))) {
    panic("rootfs: pipenew failed!\n");
  }

  if (!pipenew(&(c2[0]), &(c2[1]))) {
    panic("rootfs: pipenew failed!\n");
  }

  rootfsbinding = bindingnew(c1[1], c2[0], ATTR_wr|ATTR_rd|ATTR_dir);
  if (rootfsbinding == nil) {
    panic("rootfs: bindingnew failed!\n");
  }

  chanfree(c1[1]);
  chanfree(c2[0]);
  
  pr = procnew(8);
  if (pr == nil) {
    panic("rootfs: procnew failed!\n");
  }

  forkfunc(pr, &rootfsproc, nil);

  pr->fgroup = fgroupnew();
  in = fgroupaddchan(pr->fgroup, c1[0]);
  out = fgroupaddchan(pr->fgroup, c2[1]);

  pm = procnew(8);
  if (pm == nil) {
    panic("rootfs: procnew failed!\n");
  }

  forkfunc(pm, &mountproc, (void *) rootfsbinding);

  procready(pr);
  procready(pm);
}
 

static void
bgetfid(struct request_getfid *req, struct response_getfid *resp)
{
  struct file *f, *c;
  int i;

  f = fids[req->head.fid];

  for (c = f->children; c != nil; c = c->cnext) {
    if (strncmp(c->name, req->body.name, NAMEMAX)) {
      break;
    }
  }

  if (c == nil) {
    resp->head.ret = ENOCHILD;
    return;
  }

  for (i = 1; i < FIDSMAX; i++) {
    if (fids[i] == nil) {
      break;
    }
  }

  if (i == FIDSMAX) {
    resp->head.ret = ENOMEM;
    return;
  }

  fids[i] = c;
  
  resp->body.attr = c->stat.attr;
  resp->body.fid = i;
  resp->head.ret = OK;
}

static void
bclunk(struct request_clunk *req, struct response_clunk *resp)
{
  resp->head.ret = OK;
  fids[req->head.fid] = nil;
}

static void
bstat(struct request_stat *req, struct response_stat *resp)
{
  memmove(&resp->body.stat, &fids[req->head.fid]->stat,
	  sizeof(struct stat));

  resp->head.ret = OK;
}

static void
bopen(struct request_open *req, struct response_open *resp)
{
  fids[req->head.fid]->open++;
  resp->head.ret = OK;
}

static void
bclose(struct request_close *req, struct response_close *resp)
{
  fids[req->head.fid]->open--;
  resp->head.ret = OK;
}

static void
bcreate(struct request_create *req, struct response_create *resp)
{
  struct file *p, *new;
  int i;

  for (i = 1; i < FIDSMAX; i++) {
    if (fids[i] == nil) {
      break;
    }
  }

  p = fids[req->head.fid];
  
  if (!(p->stat.attr & ATTR_dir)) {
    resp->head.ret = ERR;
    return;
  }

  new = malloc(sizeof(struct file));
  if (new == nil) {
    resp->head.ret = ENOMEM;
    return;
  }

  fids[i] = new;
  
  new->stat.attr = req->body.attr;
  new->stat.size = 0;
  new->stat.dsize = sizeof(struct file);
  new->open = 0;
  new->children = nil;

  new->lname = strlcpy(new->name, req->body.name, NAMEMAX);

  new->parent = p;

  new->cnext = p->children;
  p->children = new;

  p->stat.size++;

  resp->head.ret = OK;
  resp->body.fid = i;
}

static void
bremove(struct request_remove *req, struct response_remove *resp)
{
  struct file *f, *c;

  f = fids[req->head.fid];
  if (f->open > 0) {
    resp->head.ret = ERR;
  } else if (f->children != nil) {
    resp->head.ret = ENOTEMPTY;
  }

  /* Remove from parent */
  for (c = f->parent->children;
       c != nil && c->cnext != f; c = c->cnext)
    ;

  f->parent->stat.size--;
  
  if (c == nil) {
    f->parent->children = f->cnext;
  } else {
    c->cnext = f->cnext;
  }

  free(f);
  resp->head.ret = OK;
}

static void
breaddir(struct request_read *req, struct response_read *resp,
	 struct file *file)
{
  uint32_t tlen, offset, len, l;
  struct file *c;
  uint8_t *buf;

  buf = resp->body.data;
  offset = req->body.offset;
  len = req->body.len;
  tlen = 0;

  for (c = file->children; c != nil; c = c->cnext) {
    l = c->lname;
    if (offset >= l + sizeof(uint8_t)) {
      offset -= l + sizeof(uint8_t);
      continue;
    }
    
    if (offset == 0) { 
      memmove(buf + tlen, &l, sizeof(uint8_t));
      tlen += sizeof(uint8_t);
    } else {
      offset -= sizeof(uint8_t);
    }
    
    if (tlen >= len) {
      break;
    } else if (tlen + l - offset > len) {
      l = len - tlen;
    }

    if (offset == 0) {
      memmove(buf + tlen, c->name, l);
    } else {
      l -= offset;
      memmove(buf + tlen, c->name + offset, l);
      offset = 0;
    }

    tlen += l;

    if (tlen >= len) {
      break;
    }
  }

  resp->body.len = tlen;
  if (tlen > 0) {
    resp->head.ret = OK;
  } else {
    resp->head.ret = EOF;
  }
}

static void
bread(struct request_read *req, struct response_read *resp)
{
  struct file *f;

  f = fids[req->head.fid];
  if (f->stat.attr & ATTR_dir) {
    breaddir(req, resp, f);
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

  root->lname = 0;

  root->stat.attr = ATTR_rd|ATTR_wr|ATTR_dir;
  root->stat.size = 0;
  root->stat.dsize = sizeof(struct file);
  root->open = 0;
  root->cnext = nil;
  root->parent = nil;
  root->children = nil;

  fids[ROOTFID] = root;
  
  rootmount.databuf = malloc(512);
  rootmount.buflen = 512;

  return fsmountloop(in, out, &rootmount);
}
