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

struct cfile {
  struct bindingfid *fid;
  uint32_t offset;
};

static struct response *
makereq(struct binding *b, struct request *req)
{
  size_t s = sizeof(req->rid)
    + sizeof(req->type)
    + sizeof(req->fid)
    + sizeof(req->lbuf);

  req->rid = atomicinc(&b->nreqid);

  lock(&b->lock);

  if (b->out == nil ||
      pipewrite(b->out, (void *) req, s) != s || 
      (req->lbuf > 0 &&
       pipewrite(b->out, req->buf, req->lbuf) != req->lbuf)) {

    unlock(&b->lock);
    return nil;
  }

  up->aux = (void *) req->rid;

  setintr(INTR_OFF);

  procwait(up, &b->waiting);
  unlock(&b->lock);

  schedule();
  setintr(INTR_ON);

  return (struct response *) up->aux;
}

static void
bindingfidfree(struct bindingfid *fid)
{
  struct bindingfid *o;
  struct request req;
  struct response *resp;

  if (atomicdec(&fid->refs) > 0) {
    return;
  } else if (fid->children != nil) {
    return;
  }

  printf("can actually free it\nremove self from parent list\n");

  lock(&fid->binding->lock);
  
  if (fid->parent->children == fid) {
    fid->parent->children = fid->cnext;
  } else {
    for (o = fid->parent->children; o->cnext != fid; o = o->cnext)
      ;
    
    o->cnext = fid->cnext;
  }

  unlock(&fid->binding->lock);

  req.type = REQ_clunk;
  req.fid = fid->fid;
  req.lbuf = 0;

  printf("send clunk %i\n", fid->fid);
  
  resp = makereq(fid->binding, &req);
  if (resp != nil) {
    printf("got a response\n");
    if (resp->lbuf > 0) {
      free(resp->buf);
    }
    free(resp);
  }
  
  printf("possibly free parent\n");
  bindingfidfree(fid->parent);

  bindingfree(fid->binding);
  
  printf("free mem\n");
  free(fid);
}

static int
filestatfid(struct bindingfid *fid, struct stat *stat)
{
  struct response *resp;
  struct request req;
  int ret;

  req.type = REQ_stat;
  req.lbuf = 0;
  req.fid = fid->fid;

  resp = makereq(fid->binding, &req);
  if (resp == nil) {
    return ELINK;
  }
	
  ret = resp->ret;

  if (ret == OK && resp->lbuf == sizeof(struct stat)) {
    memmove(stat, resp->buf, resp->lbuf);
  } else {
    ret = ELINK;
  }

  if (resp->lbuf > 0) {
    free(resp->buf);
  }
  
  free(resp);

  return ret;
}

static struct bindingfid *
findfileindir(struct bindingfid *fid, char *name, int *err)
{
  struct response_fid *fidresp;
  struct bindingfid *nfid;
  struct response *resp;
  struct request req;

  for (nfid = fid->children; nfid != nil; nfid = nfid->cnext) {
    if (strncmp(nfid->name, name, NAMEMAX)) {
      *err = OK;
      return nfid;
    }
  }

  req.type = REQ_fid;
  req.fid = fid->fid;
  req.lbuf = strlen(name);
  req.buf = (uint8_t *) name;

  resp = makereq(fid->binding, &req);
  if (resp == nil) {
    *err = ELINK;
    return 0;
  } else if (resp->ret != OK) {
    *err = resp->ret;
    goto err;
  } else if (resp->lbuf != sizeof(struct response_fid)) {
    *err = ELINK;
    goto err;
 }

  fidresp = (struct response_fid *) resp->buf;

  nfid = malloc(sizeof(struct bindingfid));
  if (nfid == nil) {
    *err = ENOMEM;
    goto err;
  }

  atomicinc(&fid->refs);
  atomicinc(&fid->binding->refs);

  nfid->refs = 1;
  nfid->binding = fid->binding;
  nfid->children = nil;

  nfid->fid = fidresp->fid;
  nfid->attr = fidresp->attr;
  memmove(nfid->name, name, NAMEMAX);

  lock(&fid->binding->lock);
  
  nfid->parent = fid;
  nfid->cnext = fid->children;
  fid->children = fid;

  unlock(&fid->binding->lock);

  free(resp->buf);
  free(resp);

  return nfid;

 err:
  if (resp->lbuf > 0) {
      free(resp->buf);
  }
  
  free(resp);
  return nil;
}

struct bindingfid *
findfile(struct path *path, int *err)
{
  struct bindingl *b, *bp;
  struct bindingfid *nfid, *fid;
  struct path *p;
  size_t depth;

  depth = 0;

  bp = b = ngroupfindbindingl(up->ngroup, path, depth++);
  if (b == nil) {
    *err = ENOFILE;
    return nil;
  } else {
    *err = OK;
  }

  fid = b->rootfid;

  p = path;
  while (p != nil) {
    if (!(fid->attr & ATTR_dir)) {
      *err = ENOFILE;
      return nil;
    }

    nfid = findfileindir(fid, p->s, err);
    if (*err == ENOCHILD && p->next == nil) {
      return fid;
    } else if (*err != OK) {
      return nil;
    }

    /* Free parent binding, should not clunk it as we have a 
     * reference to its child. */
    bindingfidfree(fid);

    b = ngroupfindbindingl(up->ngroup, path, depth++);
    if (b == nil) {
      *err = ENOFILE;
      return nil;
    } else if (b != bp) {
      /* If we havent encountered this binding yet then it is the
       * root of the binding. */
      bp = b;
      fid = b->rootfid;
    } else {
      /* Use the fid from parent */
      fid = nfid;
    }

    p = p->next;
  }

  return fid;
}

static struct bindingfid *
filecreate(struct bindingfid *parent, char *name, 
	   uint32_t cattr, int *err)
{
  struct bindingfid *nfid;
  struct request_create rc;
  struct response *resp;
  struct request req;

  req.type = REQ_create;
  req.fid = parent->fid;
	
  rc.attr = cattr;
  strlcpy(rc.name, name, NAMEMAX);
  
  req.lbuf = sizeof(struct request_create);
  req.buf = (uint8_t *) &rc;
	
  resp = makereq(parent->binding, &req);
  if (resp == nil) {
    *err = ELINK;
    return nil;
  } else if (resp->ret < 0) {
    *err = resp->ret;
  } 

  nfid = malloc(sizeof(struct bindingfid));
  if (nfid == nil) {
    *err = ENOMEM;
    return nil;
  }

  atomicinc(&parent->binding->refs);
  atomicinc(&parent->refs);

  nfid->refs = 1;
  nfid->binding = parent->binding;
  nfid->children = nil;

  nfid->parent = parent;
  nfid->attr = cattr;
  nfid->fid = resp->ret;
  memmove(nfid->name, name, NAMEMAX);

  lock(&parent->binding->lock);

  nfid->cnext = parent->children;
  parent->children = nfid;

  unlock(&parent->binding->lock);

  free(resp);

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

  return filestatfid(fid, stat);
}

static bool
checkmode(uint32_t attr, uint32_t mode)
{
  return true;
}

struct chan *
fileopen(struct path *path, uint32_t mode, uint32_t cmode, int *err)
{
  struct bindingfid *fid;
  struct response *resp;
  struct request req;
  struct cfile *cfile;
  struct chan *c;
  struct path *p;

  fid = findfile(path, err);
  if (*err != OK) {
    if (*err != ENOCHILD || fid == nil || cmode == 0) {
      *err = ERR;
      return nil;
    } else if ((fid->attr & ATTR_wr) == 0) {
      *err = EMODE;
      return nil;
    } else {
      /* Create the file first. */
      for (p = path; p != nil && p->next != nil; p = p->next);
      if (p == nil) {
	return nil;
      }

      fid = filecreate(fid, p->s, cmode, err);
      if (*err != OK) {
	return nil;
      }
    }
  }

  if (!checkmode(fid->attr, mode)) {
    *err = EMODE;
    return nil;
  }

  req.type = REQ_open;
  req.lbuf = 0;
  req.fid = fid->fid;
  resp = makereq(fid->binding, &req);
  if (resp == nil) {
    *err = ELINK;
    return nil;
  }

  *err = resp->ret;

  if (resp->lbuf > 0) {
    free(resp->buf);
  }
  
  free(resp);
	
  if (*err != OK) {
    return nil;
  }

  c = channew(CHAN_file, mode);
  if (c == nil) {
    return nil;
  }
	
  cfile = malloc(sizeof(struct cfile));
  if (cfile == nil) {
    chanfree(c);
    return nil;
  }
	
  c->aux = cfile;

  atomicinc(&fid->refs);
  atomicinc(&fid->binding->refs);
	
  cfile->fid = fid;
  cfile->offset = 0;

  return c;
}

int
fileremove(struct path *path)
{
  struct bindingfid *fid;
  struct response *resp;
  struct request req;
  uint32_t f;
  int err;

  printf("file remove\n");
  
  fid = findfile(path, &err);
  if (err != OK) {
    return ENOFILE;
  }

  f = fid->fid;

  if (fid->refs != 1) {
    printf("file has other referenecs!\n");
    return ERR;
  }
  
  bindingfidfree(fid);

  printf("now send remove %i\n", f);
  
  req.type = REQ_remove;
  req.lbuf = 0;
  req.fid = f;
  resp = makereq(fid->binding, &req);
  if (resp == nil) {
    return ELINK;
  }
	
  err = resp->ret;
  if (resp->lbuf > 0)
    free(resp->buf);
  free(resp);

  return err;
}

static int
fileread(struct chan *c, uint8_t *buf, size_t n)
{
  struct request req;
  struct request_read read;
  struct response *resp;
  struct cfile *cfile;
  int ret;

  cfile = (struct cfile *) c->aux;

  req.fid = cfile->fid->fid;
  req.type = REQ_read;
  req.lbuf = sizeof(struct request_read);
  req.buf = (uint8_t *) &read;
		
  read.offset = cfile->offset;
  read.len = n;
		
  resp = makereq(cfile->fid->binding, &req);
  if (resp == nil) {
    return ELINK;
  } else if (resp->ret != OK) {
    ret = resp->ret;
  } else if (resp->lbuf > 0) {
    n = n > resp->lbuf ? resp->lbuf : n;
    memmove(buf, resp->buf, n);

    cfile->offset += n;
    
    ret = n;

    free(resp->buf);
  } else {
    ret = ELINK;
  }

  free(resp);
  return ret;
}

static int
filewrite(struct chan *c, uint8_t *buf, size_t n)
{
  struct response *resp;
  struct request req;
  struct cfile *cfile;
  uint8_t *b;
  int ret;

  cfile = (struct cfile *) c->aux;

  req.type = REQ_write;
  req.fid = cfile->fid->fid;
  req.lbuf = sizeof(uint32_t) * 2 + n;

  req.buf = malloc(req.lbuf);
  if (req.buf == nil) {
    return ENOMEM;
  }

  b = req.buf;
  memmove(b, &cfile->offset, sizeof(uint32_t));
  b += sizeof(uint32_t);
  memmove(b, &n, sizeof(uint32_t));
  b += sizeof(uint32_t);
  /* Should change this to write strait into the pipe. */
  memmove(b, buf, n);

  resp = makereq(cfile->fid->binding, &req);

  free(req.buf);
	
  if (resp == nil) {
    return ELINK;
  }

  ret = resp->ret;
  if (ret > 0) {
    cfile->offset += ret;
  } else {
  }

  if (resp->lbuf > 0) {
    free(resp->buf);
  }

  free(resp);

  return ret;
}

static int
fileseek(struct chan *c, size_t offset, int whence)
{
  struct cfile *cfile;

  cfile = (struct cfile *) c->aux;

  switch(whence) {
  case SEEK_SET:
    cfile->offset = offset;
    break;
  case SEEK_CUR:
    cfile->offset += offset;
    break;
  default:
    return ERR;
  }

  return cfile->offset;
}

static void
fileclose(struct chan *c)
{
  struct response *resp;
  struct cfile *cfile;
  struct request req;

  cfile = (struct cfile *) c->aux;
  if (cfile == nil) {
    return;
  }

  req.type = REQ_close;
  req.fid = cfile->fid->fid;
  req.lbuf = 0;

  resp = makereq(cfile->fid->binding, &req);
  if (resp != nil) {
    if (resp->lbuf > 0) {
      free(resp->buf);
    }

    free(resp);
  }

  bindingfidfree(cfile->fid);
  free(cfile);
}

struct chantype devfile = {
  &fileread,
  &filewrite,
  &fileseek,
  &fileclose,
};
