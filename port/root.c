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

#include "head.h"

struct fstree {
  struct file file;
  struct dir *dir;
  uint32_t open;
  struct fstree *next;
};

struct fstree *tree = nil;
uint32_t nfid = 0;

static int
rootproc(void *arg);

struct binding *rootbinding;

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

  c[1]->refs--;
  c[2]->refs--;
  
  pr = newproc();
  if (pr == nil) {
    panic("initroot: newproc failed!\n");
  }

  initproc(pr);
  forkfunc(pr, &rootproc, (void *) c);

  pr->fgroup = newfgroup();
  pr->ngroup = nil;
  pr->parent = nil;
	
  ps = newproc();
  if (ps == nil) {
    panic("initroot: newproc failed!\n");
  }

  initproc(ps);
  forkfunc(ps, &mountproc, (void *) rootbinding);
	
  rootbinding->srv = ps;
	
  procready(pr);
  procready(ps);

#if DEBUG == 1
  /* Can go here as debugfs will be shared for all kernel procs. */
  debugfs = true;
#endif
}

static struct fstree *
findtree(struct fstree *t, uint32_t fid)
{
  if (t == nil) {
    return nil;
  } else if (t->file.fid == fid) {
    return t;
  } else {
    return findtree(t->next, fid);
  }
}

static void
bopen(struct request *req, struct response *resp)
{
  struct fstree *t;

  t = findtree(tree, req->fid);
  if (t == nil) {
    resp->ret = ENOFILE;
  } else {
    resp->ret = OK;
    t->open++;
  }
}

static void
bclose(struct request *req, struct response *resp)
{
  struct fstree *t;

  t = findtree(tree, req->fid);
  if (t == nil) {
    resp->ret = ENOFILE;
  } else {
    resp->ret = OK;
    tree->open--;
  }
}

static void
bwalk(struct request *req, struct response *resp)
{
  struct fstree *t;

  t = findtree(tree, req->fid);
  if (t == nil) {
    resp->ret = ENOFILE;
  } else if (t->dir == nil) {
    resp->ret = EMODE;
  } else {
    resp->ret = OK;
    resp->buf = dirtowalkresponse(t->dir, &resp->lbuf);
  }
}

static void
bremove(struct request *req, struct response *resp)
{
  resp->ret = ENOIMPL;
}

static void
bcreate(struct request *req, struct response *resp)
{
  uint32_t attr, lname;
  struct fstree *t, *p;
  struct file **files;
  uint8_t *buf;
  int i;

  buf = req->buf;
  memmove(&attr, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(&lname, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);

  p = findtree(tree, req->fid);
  
  if (p == nil || p->dir == nil) {
    resp->ret = ENOFILE;
    return;
  }

  t = malloc(sizeof(struct fstree));
  tree->open = 0;
  t->file.fid = ++nfid;
  t->file.attr = attr;
  t->file.lname = lname;
  t->file.name = malloc(sizeof(uint8_t) * lname);
  memmove(t->file.name, buf, sizeof(uint8_t) * lname);

  if ((attr & ATTR_dir) == ATTR_dir) {
    t->dir = malloc(sizeof(struct dir));
    t->dir->nfiles = 0;
    t->dir->files = nil;
  } else {
    t->dir = nil;
  }

  files = malloc(sizeof(struct file *) * (p->dir->nfiles + 1));
  if (!files) {
    resp->ret = ENOMEM;
    if (t->dir) free(t->dir);
    free(t->file.name);
    free(t);
    return;
  }

  for (i = 0; i < p->dir->nfiles; i++)
    files[i] = p->dir->files[i];

  files[i] = &(t->file);

  if (p->dir->nfiles > 0)
    free(p->dir->files);

  p->dir->nfiles++;
  p->dir->files = files;

  t->next = tree;
  tree = t;

  resp->ret = OK;
  resp->lbuf = sizeof(uint32_t);

  resp->buf = malloc(sizeof(uint32_t));
  if (resp->buf == nil) {
    /* Should to free stuff but cant be bothered. */
    resp->ret = ERR;
    return;
  }

  memmove(resp->buf, &t->file.fid, sizeof(uint32_t));
}

static struct fsmount mount = {
  &bopen,
  &bclose,
  &bwalk,
  nil,
  nil,
  &bremove,
  &bcreate
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

  tree = malloc(sizeof(struct fstree));
  if (tree == nil) {
    panic("root tree malloc failed!\n");
  }

  tree->open = 0;
  
  tree->file.fid = ROOTFID;
  tree->file.attr = ATTR_wr|ATTR_rd|ATTR_dir;
  tree->file.name = nil;
  tree->file.lname = 0;

  tree->dir = malloc(sizeof(struct dir));
  if (tree->dir == nil) {
    panic("root tree dir malloc failed!\n");
  }

  tree->dir->nfiles = 0;
  tree->dir->files = 0;

  tree->next = nil;

  return fsmountloop(in, out, &mount);
}
