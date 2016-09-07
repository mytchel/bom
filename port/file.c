#include "head.h"

struct cfile {
  uint32_t fid;
  bool dir;
  struct binding *binding;
  uint32_t offset;
};

static struct response *
makereq(struct binding *b, struct request *req)
{
  size_t s;

  lock(&b->lock);

  req->rid = b->nreqid++;

  s = sizeof(req->rid)
    + sizeof(req->type)
    + sizeof(req->fid)
    + sizeof(req->lbuf);

  if (pipewrite(b->out, (void *) req, s) != s
      || (req->lbuf > 0 && pipewrite(b->out, req->buf, req->lbuf)
	  != req->lbuf)) {
    unlock(&b->lock);
    return nil;
  }

  current->waiting.rid = req->rid;
  current->wnext = b->waiting;
  b->waiting = current;

  unlock(&b->lock);
  procwait(current);
  schedule();

  return (struct response *) current->aux;
}

static struct file *
findfileindir(struct dir *dir, uint8_t *name)
{
  struct file *f;
  size_t i;
	
  for (i = 0; i < dir->nfiles; i++) {
    f = dir->files[i];
    if (strcmp(f->name, name)) {
      return f;
    }
  }
	
  return nil;
}

static struct binding *
filewalk(struct path *path, struct file *parent, struct file *file, int *err)
{
  struct binding *b, *bp;
  struct response *resp;
  struct request req;
  struct path *p;
  struct dir *dir;
  struct file *f;
  size_t depth;
	
  req.type = REQ_walk;
  req.lbuf = 0;
  req.fid = ROOTFID;
	
  b = findbinding(current->ngroup, path, 0);
  if (b == nil) {
    *err = ENOFILE;
    return nil;
  }
	
  bp = nil;
  p = path;
  depth = 0;
  *err = OK;
  while (p != nil) {
    resp = makereq(b, &req);
    if (resp == nil) {
      *err = ELINK;
      return nil;
    } else if (resp->ret != OK) {
      *err = resp->ret;
      if (resp->lbuf > 0)
	free(resp->buf);
      free(resp);
      return nil;
    } else if (resp->lbuf == 0) {
      free(resp);
      *err = ELINK;
      return nil;
    }
		
    dir = walkresponsetodir(resp->buf, resp->lbuf);
    if (dir == nil) {
      *err = ELINK;
      return nil;
    }
		
    f = findfileindir(dir, p->s);
    if (f == nil) {
      *err = ENOFILE;
    } else if (p->next) {
      if ((f->attr & ATTR_dir) == 0) {
	/* Part of path not a dir, so no such file. */
	*err = ENOFILE;
      } else if (p->next->next == nil) {
	*err = OK;
	parent->attr = f->attr;
	parent->fid = f->fid;
      } else {
	*err = OK;
      }
			
      req.fid = f->fid;
    } else {
      file->attr = f->attr;
      file->fid = f->fid;
      *err = OK;
    }
		
    free(resp->buf);
    free(resp);
		
    if (*err != OK) {
      return b;
    }
		
    p = p->next;

    if (p == nil)
      break;
		
    bp = b;
    b = findbinding(current->ngroup, path, ++depth);
    if (b == nil) {
      *err = ENOFILE;
      return nil;
    } else if (b != bp) {
      /* If we havent encountered this binding yet then it is the
       * root of the binding. */
      req.fid = b->rootfid;
    }
  }
	
  return b;
}

static uint32_t
filecreate(struct binding *b, uint32_t pfid, uint8_t *name, 
	   uint32_t cattr, int *err)
{
  struct response *resp;
  struct request req;
  uint8_t *buf;
  uint32_t cfid;
  int nlen;
	
  req.type = REQ_create;
  req.fid = pfid;
	
  nlen = strlen(name) + 1;
	
  req.lbuf = sizeof(uint32_t) * 2 + sizeof(uint8_t) * nlen;
  buf = req.buf = malloc(req.lbuf);
	
  memmove(buf, &cattr, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(buf, &nlen, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(buf, name, sizeof(uint8_t) * nlen);
	
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

struct chan *
fileopen(struct path *path, uint32_t mode, uint32_t cmode, int *err)
{
  struct file parent, file;
  struct path *p;
  struct binding *b;
  struct response *resp;
  struct request req;
  struct cfile *cfile;
  struct chan *c;
  uint32_t attr;
	
  req.type = REQ_open;
  req.lbuf = 0;
	
  b = filewalk(path, &parent, &file, err);
  if (*err != OK) {
    if (b == nil || cmode == 0) {
      return nil;
    } else {
      /* Create the file first. */
      for (p = path; p != nil && p->next != nil; p = p->next);
      if (p == nil) {
	return nil;
      }
			
      req.fid = filecreate(b, parent.fid, p->s, cmode, err);
      attr = cmode;
      if (*err != OK) {
	return nil;
      }
    }
  } else {
    req.fid = file.fid;
    attr = file.attr;
  }
	
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
	
  c = newchan(CHAN_file, mode, path);
  if (c == nil) {
    return nil;
  }
	
  cfile = malloc(sizeof(struct cfile));
  if (cfile == nil) {
    freechan(c);
    return nil;
  }
	
  c->aux = cfile;
	
  lock(&b->lock);
  b->refs++;
  unlock(&b->lock);
	
  cfile->fid = req.fid;
  cfile->binding = b;
  cfile->offset = 0;
  if (attr & ATTR_dir)
    cfile->dir = true;
  else
    cfile->dir = false;

  return c;
}

int
fileremove(struct path *path)
{
  struct binding *b;
  struct response *resp;
  struct request req;
  struct file parent, file;
  int err;
	
  b = filewalk(path, &parent, &file, &err);
  if (err != OK) {
    return ENOFILE;
  }

  req.type = REQ_remove;
  req.lbuf = 0;
  req.fid = file.fid;
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

int
fileread(struct chan *c, uint8_t *buf, size_t n)
{
  struct request req;
  struct request_read read;
  struct response *resp;
  struct cfile *cfile;
  int err = 0;
	
  cfile = (struct cfile *) c->aux;

  req.fid = cfile->fid;
  if (cfile->dir) {
    req.type = REQ_walk;
    req.lbuf = 0;

    resp = makereq(cfile->binding, &req);
    if (resp == nil) {
      return ELINK;
    } else if (resp->ret == OK && resp->lbuf <= cfile->offset) {
      err = EOF;
    } else if (resp->ret == OK) {
      if (resp->lbuf - cfile->offset < n)
	n = resp->lbuf - cfile->offset;
				
      memmove(buf, resp->buf + cfile->offset, n);
      cfile->offset += n;
      err = n;
    } else {
      err = resp->ret;
    }
  } else {
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
    } else {
      err = ELINK;
    }
  }

  if (resp->lbuf > 0)
    free(resp->buf);
  free(resp);
  return err;
}

int
filewrite(struct chan *c, uint8_t *buf, size_t n)
{
  struct request req;
  struct response *resp;
  struct cfile *cfile;
  uint8_t *b;
  int err;
	
  cfile = (struct cfile *) c->aux;
	
  req.type = REQ_write;
  req.fid = cfile->fid;
  req.lbuf = sizeof(uint32_t) * 2 + sizeof(uint8_t) * n;
  req.buf = b = malloc(req.lbuf);
	
  memmove(b, &cfile->offset, sizeof(uint32_t));
  b += sizeof(uint32_t);
  memmove(b, &n, sizeof(uint32_t));
  b += sizeof(uint32_t);
  /* Should change this to write strait into the pipe. */
  memmove(b, buf, sizeof(uint8_t) * n);

  resp = makereq(cfile->binding, &req);
  free(req.buf);
	
  if (resp == nil) {
    return ELINK;
  } if (resp->ret != OK) {
    err = resp->ret;
  } else if (resp->lbuf == sizeof(uint32_t)) {
    memmove(&n, resp->buf, sizeof(uint32_t));
    cfile->offset += n;
    err = n;
  } else {
    err = ELINK;
  }

  if (resp->lbuf > 0)
    free(resp->buf);
  free(resp);
  return err;
}

int
fileclose(struct chan *c)
{
  struct cfile *cfile;
  struct request req;
  struct response *resp;
  int err;
	
  cfile = (struct cfile *) c->aux;

  req.type = REQ_close;
  req.fid = cfile->fid;
  req.lbuf = 0;

  resp = makereq(cfile->binding, &req);
  if (resp == nil) {
    return ELINK;
  }
	
  err = resp->ret;
  if (resp->lbuf > 0)
    free(resp->buf);
  free(resp);

  if (err == OK) {	
    lock(&cfile->binding->lock);
    cfile->binding->refs--;
    unlock(&cfile->binding->lock);
		
    free(cfile);
  }
	
  return err;
}

struct chantype devfile = {
  &fileread,
  &filewrite,
  &fileclose,
};
