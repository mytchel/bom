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

/* NOT PROPERLY IMPLIMENTED */

#include "head.h"

typedef enum { PROC_root, PROC_dir,
	       PROC_state, PROC_note,
} pfile_t;

struct file {
  uint32_t open;

  uint32_t fid;
  struct stat stat;
  uint8_t name[NAMEMAX];
  uint8_t lname;

  struct proc *proc;

  pfile_t type;

  struct file *next;
};

static int
procfsproc(void *arg);

static uint32_t nfid = ROOTFID;
static struct file *files = nil;

static struct stat rootstat = {
  ATTR_rd|ATTR_dir,
  0,
};

static uint8_t *rootbuf = nil, *procbuf = nil;
static size_t lrootbuf = 0, lprocbuf = 0;

static struct chan *fsin;

struct binding *procfsbinding;

void
procfsinit(void)
{
  struct chan *out;
  struct proc *p;

  return;

  if (!pipenew(&(fsin), &(out))) {
    panic("procfs: pipenew failed!\n");
  }

  procfsbinding = bindingnew(out, nil);
  if (procfsbinding == nil) {
    panic("procfs: bindingnew failed!\n");
  }

  p = procnew(60);
  if (p == nil) {
    panic("procfs: procnew failed!\n");
  }

  forkfunc(p, &procfsproc, nil);
  procready(p);
}

static int
adddirtoroot(struct file *n)
{
  uint8_t *buf, *tbuf;

  buf = malloc(lrootbuf + sizeof(uint8_t) + n->lname);
  if (buf == nil) {
    return ENOMEM;
  }

  tbuf = buf;
  if (lrootbuf > 0) {
    memmove(tbuf, rootbuf, lrootbuf);
    tbuf += lrootbuf;
    free(rootbuf);
  }
  
  memmove(tbuf, &n->lname, sizeof(uint8_t));
  tbuf += sizeof(uint8_t);
  memmove(tbuf, n->name, sizeof(uint8_t) * n->lname);

  rootbuf = buf;
  lrootbuf = lrootbuf + sizeof(uint8_t) * (1 + n->lname);
  
  rootstat.size++;

  return OK;
}

struct file *
addfile(char *name, pfile_t type, struct proc *p)
{
  struct file *f;
  
  f = malloc(sizeof(struct file));

  f->lname = strlen(name);
  memmove(f->name, name, f->lname);

  f->open = 0;
  f->fid = ++nfid;

  f->proc = p;
  f->type = type;

  f->next = files;
  files = f;

  return f;
}

void
procfsaddproc(struct proc *proc)
{
  struct file *dir, *note, *state;
  char name[NAMEMAX];

  return;

  snprintf(name, NAMEMAX, "%i", proc->pid);

  dir = addfile(name, PROC_dir, proc);

  adddirtoroot(dir);
  
  dir->stat.attr = ATTR_rd|ATTR_dir;
  dir->stat.size = 2;

  note = addfile("note", PROC_note, proc);
  note->stat.attr = ATTR_rd|ATTR_wr;
  note->stat.size = 0;

  state = addfile("state", PROC_state, proc);
  state->stat.attr = ATTR_rd|ATTR_wr;
  state->stat.size = 0;

}

void
procfsrmproc(struct proc *proc)
{

}

static struct file *
findfile(uint32_t fid)
{
  struct file *p;
  
  for (p = files; p != nil; p = p->next) {
    if (p->fid == fid) {
      return p;
    }
  }

  return nil;
}

static struct file *
findsubfile(struct proc *proc, pfile_t type)
{
  struct file *p;

  for (p = files; p != nil; p = p->next) {
    if (p->type == type && p->proc == proc) {
      return p;
    }
  }

  return nil;
}

static void
bfid(struct request *req, struct response *resp)
{
  struct file *p;
  uint32_t fid;

  printf("procfs fid\n");

  if (req->fid == ROOTFID) {
    for (p = files; p != nil; p = p->next) {
      if (p->type == PROC_dir) {
	if (strncmp((char *) p->name, (char *) req->buf,
		    req->lbuf)) {
	  fid = p->fid;
	}
      }
    }
  } else {
    p = findfile(req->fid);
    if (p == nil) {
      resp->ret = ENOFILE;
      return;
    }

    if (strncmp("state", (char *) req->buf, req->lbuf)) {
      p = findsubfile(p->proc, PROC_state);
    } else if (strncmp("note", (char *) req->buf, req->lbuf)) {
      p = findsubfile(p->proc, PROC_note);
    } else {
      resp->ret = ENOFILE;
      return;
    }

    if (p != nil) {
      fid = p->fid;
    } else {
      resp->ret = ENOFILE;
      return;
    }
  }

  resp->buf = malloc(sizeof(uint32_t));
  if (resp->buf == nil) {
    resp->ret = ENOMEM;
    return;
  }
  
  resp->lbuf = sizeof(uint32_t);
  memmove(resp->buf, &fid, sizeof(uint32_t));
  resp->ret = OK;
}

static void
bstat(struct request *req, struct response *resp)
{
  struct stat *s = nil;
  struct file *f;

  if (req->fid == ROOTFID) {
    s = &rootstat;
  } else {
    f = findfile(req->fid);
    if (f != nil) {
      s = &f->stat;
    }
  }

  if (s == nil) {
    resp->ret = ENOFILE;
    return;
  }
  
  resp->buf = malloc(sizeof(struct stat));
  if (resp->buf == nil) {
    resp->ret = ENOMEM;
  } else {
    resp->ret = OK;
    resp->lbuf = sizeof(struct stat);
    memmove(resp->buf, s, sizeof(struct stat));
  }
}

static void
bopen(struct request *req, struct response *resp)
{
  struct file *f;

  if (req->fid == ROOTFID) {
    resp->ret = OK;
  } else {
    f = findfile(req->fid);
    if (f == nil) {
      resp->ret = ENOFILE;
    } else {
      resp->ret = OK;
      f->open++;
    }
  }
}

static void
bclose(struct request *req, struct response *resp)
{
  struct file *f;

  if (req->fid == ROOTFID) {
    resp->ret = OK;
  } else {
    f = findfile(req->fid);
    if (f == nil) {
      resp->ret = ENOFILE;
    } else {
      resp->ret = OK;
      f->open--;
    }
  }
}

static void
breaddir(struct request *req, struct response *resp,
	 uint8_t *buf, size_t lbuf,
	 uint32_t offset, uint32_t len)
{
  if (offset + len > lbuf) {
    len = lbuf - offset;
  }
  
  resp->buf = malloc(len);
  if (resp->buf == nil) {
    resp->ret = ENOMEM;
    return;
  }

  memmove(resp->buf, buf + offset, len);
  resp->lbuf = len;
  resp->ret = OK;
}

static void
bread(struct request *req, struct response *resp)
{
  struct file *f;
  uint32_t offset, len;
  uint8_t *buf;

  printf("procfs read\n");

  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(&len, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);

  if (req->fid == ROOTFID) {
    breaddir(req, resp, rootbuf, lrootbuf, offset, len);
    return;
  }
  
  f = findfile(req->fid);
  if (f == nil) {
    resp->ret = ENOFILE;
  } else if (f->type == PROC_dir) {
    breaddir(req, resp, procbuf, lprocbuf, offset, len);
  } else {
    resp->ret = ENOIMPL;
  }
}

static void
bwrite(struct request *req, struct response *resp)
{
  struct file *f;
  uint32_t offset;
  uint8_t *buf;

  printf("procfs write\n");

  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);

  f = findfile(req->fid);
  if (f == nil) {
    resp->ret = ENOFILE;
  } else {
    resp->ret = ENOIMPL;
  }
}

static struct fsmount mount = {
  bfid,
  bstat,
  bopen,
  bclose,
  nil,
  nil,
  bread,
  bwrite,
  nil
};

static int
procfsproc(void *arg)
{
  uint8_t *buf;
  uint8_t lnote, lstate;

  lnote = strlen("note");
  lstate = strlen("state");
  
  lprocbuf = 0
    + 1 + lnote
    + 1 + lstate;
  
  procbuf = malloc(lprocbuf);
  if (procbuf == nil) {
    panic("procfs: proc buf malloc failed.\n");
  }

  buf = procbuf;
  
  buf[0] = lnote;
  memmove(buf + 1, "note", lnote);
  buf += 1 + lnote;
  buf[0] = lstate;
  memmove(buf + 1, "state", lstate);
  
  return kmountloop(fsin, procfsbinding, &mount);
}
