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
#include <fs.h>
#include <stdarg.h>
#include <string.h>

struct file {
  uint32_t fid;

  uint8_t lname;
  char name[FS_LNAME_MAX];

  uint8_t *buf;
  uint32_t usize;

  struct stat stat;
  uint32_t open;

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

  printf("tmp close %i\n", req->fid);
  
  f = findfile(root, req->fid);
  if (f == nil) {
    resp->ret = ENOFILE;
  } else {
    printf("tmp closing %i, from open = %i\n", f->fid, f->open);
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
  new->usize = 0;
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

  p->stat.size += sizeof(uint8_t) + (1 + lname);
  new->cnext = p->children;
  p->children = new;
  
  resp->ret = OK;
  resp->lbuf = sizeof(uint32_t);

  memmove(resp->buf, &new->fid, sizeof(uint32_t));
}

static void
bremove(struct request *req, struct response *resp)
{
  struct file *f, *p, *c;

  printf("tmp remove %i\n", req->fid);
  
  f = findfile(root, req->fid);
  if (f == nil) {
    resp->ret = ENOFILE;
    return;
  } else if (f->open > 0) {
    printf("%i open, not removeing\n", req->fid);
    resp->ret = ERR;
    return;
  } else if (f->children != nil) {
    printf("%i not empty\n", req->fid);
    resp->ret = ENOTEMPTY;
    return;
  }

  p = f->parent;

  /* Remove from parent */
  for (c = p->children; c != nil && c->cnext != f; c = c->cnext)
    ;

  if (c == nil) {
    p->children = f->cnext;
  } else {
    c->cnext = f->cnext;
  }

  p->stat.size -= sizeof(uint8_t) * (1 + f->lname);

  if (f->buf != nil) {
    printf("%i remove buf\n", req->fid);
    rmmem(f->buf, f->stat.size);
  }

  printf("%i free\n", req->fid);
  free(f);
  
  resp->ret = OK;
}

static void
breaddir(struct request *req, struct response *resp,
	 struct file *file,
	 uint32_t offset, uint32_t len)
{
  struct file *c;
  uint8_t *buf;
  
  buf = malloc(file->stat.size);
  if (buf == nil) {
    resp->ret = ENOMEM;
    return;
  }

  resp->buf = buf;
  for (c = file->children; c != nil; c = c->cnext) {
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

static void
breadfile(struct request *req, struct response *resp,
	 struct file *file,
	 uint32_t offset, uint32_t len)
{
  uint32_t rlen;

  if (offset + len > file->usize) {
    resp->ret = EOF;
    return;
  }
  
  if (len > file->stat.size) {
    rlen = file->stat.size - offset;
  } else {
    rlen = len;
  }

  resp->buf = malloc(rlen);
  if (resp->buf == nil) {
    resp->ret = ENOIMPL;
    return;
  }

  memmove(resp->buf, file->buf + offset, rlen);
  
  resp->lbuf = rlen;
  resp->ret = OK;
}

static void
bread(struct request *req, struct response *resp)
{
  struct file *f;
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
  } else if (offset >= f->stat.size) {
    resp->ret = EOF;
  } else if (f->stat.attr & ATTR_dir) {
    breaddir(req, resp, f, offset, len);
  } else {
    breadfile(req, resp, f, offset, len);
  }
}

static void
bwrite(struct request *req, struct response *resp)
{
  struct file *f;
  uint32_t offset, len;
  uint8_t *buf, *nbuf;
  size_t nsize;

  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);

  len = req->lbuf - sizeof(uint32_t);

  f = findfile(root, req->fid);
  if (f == nil) {
    resp->ret = ENOFILE;
  }

  resp->buf = malloc(sizeof(uint32_t));
  if (resp->buf == nil) {
    resp->ret = ENOMEM;
    return;
  }
  resp->lbuf = sizeof(uint32_t);

  if (offset + len > f->stat.size) {
    nsize = offset + len;
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

  if (offset + len > f->usize) {
    f->usize = offset + len;
  }

  memmove(f->buf + offset, buf, len);
  memmove(resp->buf, &len, sizeof(uint32_t));
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
  &bwrite,
  nil,
};

int
tmpmount(char *path)
{
  int f, fd, p1[2], p2[2];
  
  if (pipe(p1) == ERR) {
    return -1;
  } else if (pipe(p2) == ERR) {
    return -1;
  }

  fd = open(path, O_WRONLY|O_CREATE, ATTR_wr|ATTR_rd|ATTR_dir);
  if (fd < 0) {
    return -2;
  }

  if (bind(p1[1], p2[0], path) == ERR) {
    return -3;
  }
	
  close(p1[1]);
  close(p2[0]);

  root = malloc(sizeof(struct file));
  if (root == nil) {
    printf("tmp root tree malloc failed!\n");
    return -4;
  }

  root->fid = nfid++;
  root->lname = 0;

  root->stat.attr = ATTR_rd|ATTR_wr|ATTR_dir;
  root->stat.size = 0;
  root->open = 0;
  root->cnext = nil;
  root->parent = nil;
  root->children = nil;

  f = fork(FORK_sngroup);
  if (f < 0) {
    return -5;
  } else if (!f) {
    return fsmountloop(p1[0], p2[1], &mount);
  }
	
  close(p1[0]);
  close(p2[1]);
  close(fd);
  
  return f;
}
