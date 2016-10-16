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
  struct binding *binding;
  uint32_t fid;
  uint32_t offset;
  struct stat stat;
};

static struct response *
makereq(struct binding *b, struct request *req)
{
  size_t s = sizeof(req->rid)
    + sizeof(req->type)
    + sizeof(req->fid)
    + sizeof(req->lbuf);

  lock(&b->lock);

  req->rid = b->nreqid++;

  if (pipewrite(b->out, (void *) req, s) != s || 
      (req->lbuf > 0 &&
       pipewrite(b->out, req->buf, req->lbuf) != req->lbuf)) {
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

static int
filestatfid(struct binding *b, uint32_t fid, struct stat *stat)
{
  struct response *resp;
  struct request req;
  int err;
	
  req.type = REQ_stat;
  req.lbuf = 0;
  req.fid = fid;

  resp = makereq(b, &req);
  if (resp == nil) {
    return ELINK;
  }
	
  err = resp->ret;

  if (err == OK && resp->lbuf == sizeof(struct stat)) {
    memmove(stat, resp->buf, resp->lbuf);
  } else {
    err = ELINK;
  }

  if (resp->lbuf > 0)
    free(resp->buf);
  free(resp);

  return err;
}

static uint32_t
findfileindir(struct binding *b, uint32_t fid, struct stat *stat,
	      char *name, int *err)
{
  struct response *resp;
  struct request req;
  uint32_t f;

  *err = filestatfid(b, fid, stat);
  if (*err != OK) {
    return 0;
  } else if (!(stat->attr & ATTR_dir)) {
    *err = ENOFILE;
    return 0;
  }

  req.type = REQ_fid;
  req.fid = fid;
  req.lbuf = strlen(name);
  req.buf = (uint8_t *) name;

  resp = makereq(b, &req);
  if (resp == nil) {
    *err = ELINK;
    return 0;
  } else if (resp->ret != OK) {
    *err = resp->ret;
    goto err;
  } else if (resp->lbuf != sizeof(uint32_t)) {
    *err = ELINK;
    goto err;
 }

  f = *resp->buf;

  free(resp->buf);
  free(resp);

  return f;

 err:
  if (resp->lbuf > 0)
      free(resp->buf);
  free(resp);
  return 0;
}

static struct binding *
findfile(struct path *path,
	 uint32_t *fid, struct stat *stat,
	 int *err)
{
  struct binding *b, *bp;
  struct path *p;
  uint32_t f, fr;
  size_t depth;

  bp = nil;
  b = ngroupfindbinding(up->ngroup, path, 0, &f);
  if (b == nil) {
    *err = ENOFILE;
    return nil;
  }

  if (path == nil) {
    *err = filestatfid(b, f, stat);
    if (*err != OK) {
      return nil;
    } else {
      *fid = f;
      return b;
    }
  }

  *err = OK;
  p = path;
  depth = 0;
  f = f;
  *fid = f;
  
  while (p != nil) {
    f = findfileindir(b, f, stat, p->s, err);
    if (*err == ENOCHILD && p->next == nil) {
      return b;
    } else if (*err != OK) {
      return nil;
    }

    bp = b;
    b = ngroupfindbinding(up->ngroup, path, ++depth, &fr);
    if (b == nil) {
      *err = ENOFILE;
      return nil;
    } else if (b != bp) {
      /* If we havent encountered this binding yet then it is the
       * root of the binding. */
      f = fr;
    }

    p = p->next;
    *fid = f;
  }

  return b;
}

static uint32_t
filecreate(struct binding *b, uint32_t pfid, char *name, 
	   uint32_t cattr, int *err)
{
  struct response *resp;
  struct request req;
  uint8_t *buf;
  uint32_t cfid;
  int nlen;
	
  req.type = REQ_create;
  req.fid = pfid;
	
  nlen = strlen(name);
	
  req.lbuf = sizeof(uint32_t) * 2 + nlen;
  buf = req.buf = malloc(req.lbuf);
	
  memmove(buf, &cattr, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(buf, &nlen, sizeof(uint8_t));
  buf += sizeof(uint8_t);
  memmove(buf, name, nlen);

  resp = makereq(b, &req);
  if (resp == nil) {
    *err = ELINK;
    return nil;
  } else if (resp->ret != OK) {
    *err = resp->ret;
  } else if (resp->lbuf != sizeof(uint32_t)) {
    *err = ELINK;
  } else {
    *err = OK;
    memmove(&cfid, resp->buf, sizeof(uint32_t));
  }

  if (resp->lbuf > 0)
    free(resp->buf);
  free(resp);
	
  return cfid;
}

int 
filestat(struct path *path, struct stat *stat)
{
  struct stat pstat;
  struct binding *b;
  uint32_t fid;
  int err;
  
  b = findfile(path, &fid, &pstat, &err);
  if (err != OK) {
    return err;
  }

  return filestatfid(b, fid, stat);
}

static bool
checkmode(struct stat *stat, uint32_t mode)
{
  return true;
}

struct chan *
fileopen(struct path *path, uint32_t mode, uint32_t cmode, int *err)
{
  uint32_t fid;
  struct response *resp;
  struct request req;
  struct cfile *cfile;
  struct stat stat;
  struct binding *b;
  struct chan *c;
  struct path *p;

  b = findfile(path, &fid, &stat, err);
  if (*err != OK) {
    if (*err != ENOCHILD || b == nil || cmode == 0) {
      *err = ERR;
      return nil;
    } else if ((stat.attr & ATTR_wr) == 0) {
      *err = EMODE;
      return nil;
    } else {
      /* Create the file first. */
      for (p = path; p != nil && p->next != nil; p = p->next);
      if (p == nil) {
	return nil;
      }

      fid = filecreate(b, fid, p->s, cmode, err);
      if (*err != OK) {
	return nil;
      }

      *err = filestatfid(b, fid, &stat);
      if (*err != OK) {
	return nil;
      } 
    }
  }

  if (!checkmode(&stat, mode)) {
    *err = EMODE;
    return nil;
  }

  req.type = REQ_open;
  req.lbuf = 0;
  req.fid = fid;
  resp = makereq(b, &req);
  if (resp == nil) {
    *err = ELINK;
    return nil;
  }
	
  *err = resp->ret;

  if (resp->lbuf > 0)
    free(resp->buf);	
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
	
  atomicinc(&b->refs);
	
  cfile->fid = fid;
  cfile->binding = b;
  cfile->offset = 0;
  memmove(&cfile->stat, &stat, sizeof(struct stat));

  return c;
}

int
fileremove(struct path *path)
{
  struct response *resp;
  struct request req;
  struct binding *b;
  struct stat stat;
  uint32_t fid;
  int err;
	
  b = findfile(path, &fid, &stat, &err);
  if (err != OK) {
    return ENOFILE;
  }

  req.type = REQ_remove;
  req.lbuf = 0;
  req.fid = fid;
  resp = makereq(b, &req);

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
  int err = 0;
	
  cfile = (struct cfile *) c->aux;

  req.fid = cfile->fid;
  req.type = REQ_read;
  req.lbuf = sizeof(struct request_read);
  req.buf = (uint8_t *) &read;
		
  read.offset = cfile->offset;
  read.len = n;
		
  resp = makereq(cfile->binding, &req);
  if (resp == nil) {
    return ELINK;
  } else if (resp->ret != OK) {
    err = resp->ret;
  } else if (resp->lbuf > 0) {

    n = n > resp->lbuf ? resp->lbuf : n;
    memmove(buf, resp->buf, n);

    cfile->offset += n;
    
    err = n;

    free(resp->buf);
  } else {
    err = ELINK;
  }

  free(resp);
  return err;
}

static int
filewrite(struct chan *c, uint8_t *buf, size_t n)
{
  struct request req;
  struct response *resp;
  struct cfile *cfile;
  uint8_t *b;
  uint32_t wrote;
  int err;

  cfile = (struct cfile *) c->aux;

  req.type = REQ_write;
  req.fid = cfile->fid;
  req.lbuf = sizeof(uint32_t) + n;

  req.buf = malloc(req.lbuf);
  if (req.buf == nil) {
    return ENOMEM;
  }

  b = req.buf;
  memmove(b, &cfile->offset, sizeof(uint32_t));
  b += sizeof(uint32_t);
  /* Should change this to write strait into the pipe. */
  memmove(b, buf, n);

  resp = makereq(cfile->binding, &req);

  free(req.buf);
	
  if (resp == nil) {
    return ELINK;
  } if (resp->ret != OK) {
    err = resp->ret;
  } else if (resp->lbuf == sizeof(uint32_t)) {
    memmove(&wrote, resp->buf, sizeof(uint32_t));
    cfile->offset += wrote;
    err = wrote;
  } else {
    err = ELINK;
  }

  if (resp->lbuf > 0)
    free(resp->buf);
  free(resp);

  return err;
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
  case SEEK_END:
    if (cfile->offset > cfile->stat.size) {
      cfile->stat.size = cfile->offset;
    }

    cfile->offset = cfile->stat.size - offset;
    break;
  default:
    return ERR;
  }

  return cfile->offset;
}

static int
fileclose(struct chan *c)
{
  struct response *resp;
  struct cfile *cfile;
  struct request req;

  cfile = (struct cfile *) c->aux;
  if (cfile == nil) {
    return OK;
  }

  req.type = REQ_close;
  req.fid = cfile->fid;
  req.lbuf = 0;

  resp = makereq(cfile->binding, &req);
  if (resp != nil) {
    if (resp->lbuf > 0) {
      free(resp->buf);
    }

    free(resp);
  }

  atomicdec(&cfile->binding->refs);
  free(cfile);
	
  return OK;
}

struct chantype devfile = {
  &fileread,
  &filewrite,
  &fileseek,
  &fileclose,
};
