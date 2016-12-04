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

struct fstransaction {
  bool served;
  size_t len;
  uint32_t rid, type;
  struct response *resp;
  struct fstransaction *next;
  struct proc *proc;
};

struct cfile {
  struct lock lock;
  struct bindingfid *fid;
  uint32_t offset;
};

static bool
getresponse(struct binding *b, struct fstransaction *want)
{
  if (!want->served) {
    procwait();
  }

  return true;
}

int
mountproc(void *arg)
{
  struct fstransaction *t, *p;
  struct response tresp;
  struct binding *b;
  
  b = (struct binding *) arg;

  while (b->refs > 1) {
    if (piperead(b->in, (void *) &tresp, sizeof(tresp)) < 0) {
      break;
    }

  findandremove:
    p = nil;
    for (t = b->waiting; t != nil; p = t, t = t->next) {
      if (t->rid != tresp.head.rid) {
	continue;
      } else if (p == nil) {
	if (!cas(&b->waiting, t, t->next)) {
	  goto findandremove;
	}
      } else {
	if (!cas(&p->next, t, t->next)) {
	  goto findandremove;
	}
      }

      break;
    }

    if (t == nil) {
      printf("kproc mount: response %i has no waiter.\n",
	     tresp.head.rid);
      break;
    } 

    memmove(t->resp, &tresp, t->len);

    if (t->type == REQ_read && t->resp->head.ret == OK) {
      t->resp->read.len = piperead(b->in, t->resp->read.data,
				   t->resp->read.len);

      if (t->resp->read.len < 0) {
	printf("kproc mount: error reading read response.\n");
	break;
      }
    }

    t->served = true;
    procready(t->proc);
  }

  lock(&b->lock);

  if (b->in != nil) {
    chanfree(b->in);
    b->in = nil;
  }

  if (b->out != nil) {
    chanfree(b->out);
    b->out = nil;
  }

  /* Wake up any waiting processes so they can error. */

  t = b->waiting;
  while (t != nil) {
    p = t->next;
    
    t->resp->head.ret = ELINK;
    procready(t->proc);

    t = p;
  }

  unlock(&b->lock);
  bindingfree(b);

  return ERR;
}

static bool
makereq(struct binding *b,
	struct request *req, size_t reqlen,
	struct response *resp, size_t resplen)
{
  struct fstransaction trans;
  
  if (b->out == nil) {
    return false;
  }

  trans.rid = atomicinc(&b->nreqid);
  trans.type = req->head.type;
  trans.len = resplen;
  trans.resp = resp;
  trans.proc = up;
  trans.served = false;

  req->head.rid = trans.rid;

  do {
    trans.next = b->waiting;
  } while (!cas(&b->waiting, trans.next, &trans));
  
  lock(&b->lock);

  if (pipewrite(b->out, (void *) req, reqlen) != reqlen) {
    unlock(&b->lock);
    return false;
  }

  unlock(&b->lock);
  return getresponse(b, &trans);
}

void
bindingfidfree(struct bindingfid *fid)
{
  struct bindingfid *p, *o;
  struct request_clunk req;
  struct response_clunk resp;

  if (atomicdec(&fid->refs) > 0) {
    return;
  }

  req.head.type = REQ_clunk;
  req.head.fid = fid->fid;

  makereq(fid->binding,
	  (struct request *) &req, sizeof(req),
	  (struct response *) &resp, sizeof(resp));

  if (fid->parent) {
  remove:
    p = nil;
    for (o = fid->parent->children; o != nil; p = o, o = o->cnext) {
      if (o != fid) continue;
      
      if (p == nil) {
	if (!cas(&fid->parent->children, fid, fid->cnext)) {
	  goto remove;
	}
      } else {
	if (!cas(&p->cnext, fid, fid->cnext)) {
	  goto remove;
	}
      }

      break;
    }

    bindingfidfree(fid->parent);
  } else if (fid->parent == nil) {
    bindingfree(fid->binding);
  }
 
  free(fid);
}

static int
filestatfid(struct bindingfid *fid, struct stat *stat)
{
  struct request_stat req;
  struct response_stat resp;

  req.head.type = REQ_stat;
  req.head.fid = fid->fid;

  if (!makereq(fid->binding,
	       (struct request *) &req, sizeof(req),
	       (struct response *) &resp, sizeof(resp))) {
    return ELINK;
  } else if (resp.head.ret == OK) {
    memmove(stat, &resp.body.stat, sizeof(struct stat));
    return OK;
  } else {
    return resp.head.ret;
  }
}

static struct bindingfid *
findfileindir(struct bindingfid *fid, char *name, int *err)
{
  struct request_getfid req;
  struct response_getfid resp;
  struct bindingfid *nfid;
  size_t len;

  for (nfid = fid->children; nfid != nil; nfid = nfid->cnext) {
    if (strncmp(nfid->name, name, NAMEMAX)) {
      atomicinc(&nfid->refs);
      *err = OK;
      return nfid;
    }
  }

  req.head.type = REQ_getfid;
  req.head.fid = fid->fid;

  len = sizeof(req.head);
  len += strlcpy(req.body.name, name, NAMEMAX);

  if (!makereq(fid->binding,
	       (struct request *) &req, len,
	       (struct response *) &resp, sizeof(resp))) {
    *err = ELINK;
    return nil;
  } else if (resp.head.ret != OK) {
    *err = resp.head.ret;
    return nil;
  }

  nfid = malloc(sizeof(struct bindingfid));
  if (nfid == nil) {
    *err = ENOMEM;
    return nil;
  }

  nfid->refs = 1;
  nfid->binding = fid->binding;

  nfid->fid = resp.body.fid;
  nfid->attr = resp.body.attr;
  strlcpy(nfid->name, name, NAMEMAX);

  atomicinc(&fid->refs);
  nfid->parent = fid;
  nfid->children = nil;

  do {
    nfid->cnext = fid->children;
  } while (!cas(&fid->children, nfid->cnext, nfid));

  return nfid;
}

struct bindingfid *
findfile(struct path *path, int *err)
{
  struct bindingfid *nfid, *fid;
  struct bindingl *b;
  struct path *p;

  b = ngroupfindbindingl(up->ngroup, up->root);
  if (b == nil) {
    *err = ENOFILE;
    return nil;
  } else {
    *err = OK;
  }

  fid = b->rootfid;
  atomicinc(&fid->refs);

  p = path;
  while (p != nil) {
    if (!(fid->attr & ATTR_dir)) {
      bindingfidfree(fid);
      *err = ENOFILE;
      return nil;
    }

    nfid = findfileindir(fid, p->s, err);
    if (*err == ENOCHILD && p->next == nil) {
      return fid;
    } else if (*err != OK) {
      bindingfidfree(fid);
      return nil;
    }

    /* Free parent binding, should not clunk it as we have a 
     * reference to its child. */
    bindingfidfree(fid);

    b = ngroupfindbindingl(up->ngroup, nfid);
    if (b != nil) {
      bindingfidfree(nfid);
      fid = b->rootfid;
      atomicinc(&fid->refs);
    } else {
      /* Use the fid from parent */
      fid = nfid;
    }

    p = p->next;
  }

  return fid;
}

static struct bindingfid *
filecreate(struct bindingfid *parent,
	   char *name, uint32_t cattr,
	   int *err)
{
  struct request_create req;
  struct response_create resp;
  struct bindingfid *nfid;
  size_t len;

  req.head.type = REQ_create;
  req.head.fid = parent->fid;
  req.body.attr = cattr;

  len = sizeof(req.head) + sizeof(req.body.attr);
  len += strlcpy(req.body.name, name, NAMEMAX);
  
  if (!makereq(parent->binding,
	       (struct request *) &req, len,
	       (struct response *) &resp, sizeof(resp))) {
    *err = ELINK;
    return nil;
  } else if (resp.head.ret != OK) {
    *err = resp.head.ret;
    return nil;
  } 

  nfid = malloc(sizeof(struct bindingfid));
  if (nfid == nil) {
    *err = ENOMEM;
    return nil;
  }

  nfid->refs = 1;
  nfid->binding = parent->binding;

  nfid->fid = resp.body.fid;

  nfid->attr = cattr;
  strlcpy(nfid->name, name, NAMEMAX);

  atomicinc(&parent->refs);
  nfid->parent = parent;
  nfid->children = nil;

  do {
    nfid->cnext = parent->children;
  } while (!cas(&parent->children, nfid->cnext, nfid));

  *err = OK;
  return nfid;
}

int 
filestat(struct path *path, struct stat *stat)
{
  struct bindingfid *fid;
  int err;

  fid = findfile(path, &err);
  if (err != OK) {
    return err;
  }

  err = filestatfid(fid, stat);

  bindingfidfree(fid);

  return err;
}

static bool
checkmode(uint32_t attr, uint32_t mode)
{
  if ((mode & O_WRONLY) && !(attr & ATTR_wr)) {
    return false;
  } else if ((mode & O_RDONLY) && !(attr & ATTR_rd)) {
    return false;
  } else if ((mode & O_DIR) && !(attr & ATTR_dir)) {
    return false;
  } else if ((mode & O_FILE) && (attr & ATTR_dir)) {
    return false;
  } else {
    return true;
  }
}

struct chan *
fileopen(struct path *path, uint32_t mode, uint32_t cmode, int *err)
{
  struct bindingfid *fid, *nfid;
  struct request_open req;
  struct response_open resp;
  struct cfile *cfile;
  struct chan *c;
  struct path *p;

  fid = findfile(path, err);
  if (*err != OK) {
    if (cmode == 0) {
      if (*err == ENOCHILD || fid == nil) {
	*err = ENOFILE;
      }
      return nil;
    } else if (!(fid->attr & ATTR_wr)) {
      bindingfidfree(fid);
      *err = EMODE;
      return nil;
    } else {
      /* Create the file first. */
      for (p = path; p != nil && p->next != nil; p = p->next);
      if (p == nil) {
	bindingfidfree(fid);
	return nil;
      }

      nfid = filecreate(fid, p->s, cmode, err);
      if (*err != OK) {
	bindingfidfree(fid);
	return nil;
      }

      bindingfidfree(fid);
      
      fid = nfid;
    }
  }

  if (!checkmode(fid->attr, mode)) {
    *err = EMODE;
    bindingfidfree(fid);
    return nil;
  }

  req.head.type = REQ_open;
  req.head.fid = fid->fid;
  if (!makereq(fid->binding,
	       (struct request *) &req, sizeof(req), 
	       (struct response *) &resp, sizeof(resp))) {
    *err = ELINK;
    bindingfidfree(fid);
    return nil;
  } else if (resp.head.ret != OK) {
    *err = resp.head.ret;
    bindingfidfree(fid);
    return nil;
  }

  c = channew(CHAN_file, mode);
  if (c == nil) {
    bindingfidfree(fid);
    *err = ENOMEM;
    return nil;
  }
	
  cfile = malloc(sizeof(struct cfile));
  if (cfile == nil) {
    bindingfidfree(fid);
    chanfree(c);
    *err = ENOMEM;
    return nil;
  }
	
  c->aux = cfile;

  cfile->fid = fid;
  cfile->offset = 0;

  if (mode & O_TRUNC) {
    printf("truincation not yet supported\n");
  }

  if (mode & O_APPEND) {
    printf("append not yet supported\n");
  }
  
  *err = OK;
  return c;
}

int
fileremove(struct path *path)
{
  struct bindingfid *fid;
  struct request_remove req;
  struct response_remove resp;
  int ret;

  fid = findfile(path, &ret);
  if (ret != OK) {
    return ret;
  }

  if (fid->refs != 1) {
    return ERR;
  }
  
  req.head.type = REQ_remove;
  req.head.fid = fid->fid;

  if (!makereq(fid->binding,
	       (struct request *) &req, sizeof(req),
	       (struct response *) &resp, sizeof(resp))) {
    ret = ELINK;
  } else if (resp.head.ret != OK) {
    ret =  resp.head.ret;
  } else {
    ret = OK;
  }

  bindingfidfree(fid);
  return ret;
}

int
fileread(struct chan *c, void *buf, size_t n)
{
  struct request_read req;
  struct response_read resp;
  struct cfile *cfile;

  cfile = (struct cfile *) c->aux;
  lock(&cfile->lock);

  req.head.fid = cfile->fid->fid;
  req.head.type = REQ_read;
  req.body.offset = cfile->offset;
  req.body.len = n;

  resp.body.len = n;
  resp.body.data = buf;

  if (!makereq(cfile->fid->binding,
	       (struct request *) &req, sizeof(req),
	       (struct response *) &resp, sizeof(resp.head))) {
    unlock(&cfile->lock);
    return ELINK;
  } else if (resp.head.ret != OK) {
    unlock(&cfile->lock);
    return resp.head.ret;
  } else {
    cfile->offset += resp.body.len;
    unlock(&cfile->lock);
    return resp.body.len;
  }
}

int
filewrite(struct chan *c, void *buf, size_t n)
{
  struct fstransaction trans;
  struct request_write req;
  struct response_write resp;
  struct binding *b;
  struct cfile *cfile;
  size_t reqlen;
  
  cfile = (struct cfile *) c->aux;

  lock(&cfile->lock);

  b = cfile->fid->binding;

  if (b->out == nil) {
    unlock(&cfile->lock);
    return ELINK;
  }

  trans.len = sizeof(resp);
  trans.rid = atomicinc(&b->nreqid);
  trans.type = REQ_write;
  trans.resp = (struct response *) &resp;
  trans.proc = up;
  trans.served = false;

  req.head.rid = trans.rid;
  req.head.type = REQ_write;
  req.head.fid = cfile->fid->fid;
  req.body.offset = cfile->offset;
  req.body.len = n;

  do {
    trans.next = b->waiting;
  } while (!cas(&b->waiting, trans.next, &trans));
 
  reqlen = sizeof(req.head)
    + sizeof(req.body.offset)
    + sizeof(req.body.len);
  
  lock(&b->lock);

  if (pipewrite(b->out, (void *) &req, reqlen) != reqlen) {
    unlock(&b->lock);
    unlock(&cfile->lock);
    return ELINK;
  }
  
  if (pipewrite(b->out, buf, n) < 0) {
    unlock(&b->lock);
    unlock(&cfile->lock);
    return ELINK;
  }

  unlock(&b->lock);

  if (!getresponse(b, &trans)) {
    unlock(&cfile->lock);
    return ELINK;
  } else if (resp.head.ret != OK) {
    unlock(&cfile->lock);
    return resp.head.ret;
  } else {
    cfile->offset += resp.body.len;
    unlock(&cfile->lock);
    return resp.body.len;
  }
}

int
fileseek(struct chan *c, size_t offset, int whence)
{
  struct cfile *cfile;

  cfile = (struct cfile *) c->aux;
  lock(&cfile->lock);

  switch(whence) {
  case SEEK_SET:
    cfile->offset = offset;
    break;
  case SEEK_CUR:
    cfile->offset += offset;
    break;
  default:
    unlock(&cfile->lock);
    return ERR;
  }

  unlock(&cfile->lock);
  return OK;
}

struct pagel *
getfilepages(struct chan *c, size_t offset, size_t len, bool rw)
{
  return nil;
}

void
fileclose(struct chan *c)
{
  struct request_close req;
  struct response_close resp;
  struct cfile *cfile;

  cfile = (struct cfile *) c->aux;

  lock(&cfile->lock);
  
  req.head.type = REQ_close;
  req.head.fid = cfile->fid->fid;

  makereq(cfile->fid->binding,
	  (struct request *) &req, sizeof(req),
	  (struct response *) &resp, sizeof(resp));

  bindingfidfree(cfile->fid);
  free(cfile);
}

struct chantype devfile = {
  &fileread,
  &filewrite,
  &fileseek,
  &fileclose,
};
