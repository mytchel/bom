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

reg_t
syschdir(const char *upath)
{
  struct path *path;
  struct chan *c;
  int err;

  if (kaddr(up, (void *) upath, 0) == nil) {
    return ERR;
  }

  path = strtopath(up->dot, upath);

  c = fileopen(path, O_RDONLY|O_DIR, 0, &err);
  if (err != OK) {
    pathfree(path);
    return err;
  }

  pathfree(up->dot);
  if (up->dotchan != nil) {
    chanfree(up->dotchan);
  }

  up->dotchan = c;
  up->dot = path;

  return OK;
}

int
read(int fd, void *buf, size_t len)
{
  struct chan *c;
  int r;

  c = fdtochan(up->fgroup, fd);
  if (c == nil) {
    return ERR;
  } else if (!(c->mode & O_RDONLY)) {
    return EMODE;
  }

  r = chantypes[c->type]->read(c, buf, len);

  return r;
}

reg_t
sysread(int fd, void *buf, size_t len)
{
  void *kbuf;

  kbuf = kaddr(up, buf, len);

  if (kbuf == nil) {
    return ERR;
  } else {
    return read(fd, kbuf, len);
  }
}

int
write(int fd, void *buf, size_t len)
{
  struct chan *c;
  int r;

  c = fdtochan(up->fgroup, fd);
  if (c == nil) {
    return ERR;
  } else if (!(c->mode & O_WRONLY)) {
    return EMODE;
  }
	
  r = chantypes[c->type]->write(c, buf, len);

  return r;
}

reg_t
syswrite(int fd, void *buf, size_t len)
{
  void *kbuf;

  kbuf = kaddr(up, buf, len);

  if (kbuf == nil) {
    return ERR;
  } else {
    return write(fd, kbuf, len);
  }
}

int
seek(int fd, size_t offset, int whence)
{
  struct chan *c;
  int r;

  c = fdtochan(up->fgroup, fd);
  if (c == nil) {
    return ERR;
  }
	
  r = chantypes[c->type]->seek(c, offset, whence);
	
  return r;
}

reg_t
sysseek(int fd, size_t offset, int whence)
{
  return seek(fd, offset, whence);
}
     
reg_t
sysstat(const char *upath, struct stat *ustat)
{
  const char *kpath;
  struct stat *stat;
  struct path *path;
	
  kpath = kaddr(up, (void *) upath, 0);
  if (kpath == nil) {
    return ERR;
  }
  
  stat = (struct stat *) kaddr(up, (void *) ustat,
			       sizeof(struct stat));

  if (stat == nil) {
    return ERR;
  }

  path = strtopath(up->dot, kpath);

  return filestat(path, stat);
}

reg_t
sysclose(int fd)
{
  struct chan *c;

  c = fdtochan(up->fgroup, fd);
  if (c == nil) {
    return ERR;
  }

  if (!cas(&up->fgroup->chans[fd], c, nil)) {
    return sysclose(fd);
  } else {
    chanfree(c);
    return OK;
  }
}

reg_t
sysopen(const char *upath, uint32_t mode, ...)
{
  struct path *path;
  const char *kpath;
  uint32_t cmode;
  struct chan *c;
  va_list ap;
  int err;

  kpath = kaddr(up, (void *) upath, 0);
  if (kpath == nil) {
    return ERR;
  }
  
  if (mode & O_CREATE) {
    va_start(ap, mode);
    cmode = va_arg(ap, uint32_t);
  } else {
    cmode = 0;
  }

  path = strtopath(up->dot, kpath);

  c = fileopen(path, mode, cmode, &err);

  if (c == nil) {
    pathfree(path);
    return err;
  } else {
    err = fgroupaddchan(up->fgroup, c);
    return err;
  }
}

reg_t
syspipe(int *fds)
{
  struct chan *c0, *c1;

  if (kaddr(up, fds, sizeof(int) * 2) == nil) {
    return ERR;
  }

  if (!pipenew(&c0, &c1)) {
    return ENOMEM;
  }
	
  fds[0] = fgroupaddchan(up->fgroup, c0);
  if (fds[0] < 0) {
    return fds[0];
  }
  
  fds[1] = fgroupaddchan(up->fgroup, c1);
  if (fds[1] < 0) {
    chanfree(c0);
    return fds[1];
  }
 
  return OK;
}

reg_t
sysremove(const char *upath)
{
  struct path *path;
  const char *kpath;
	
  kpath = kaddr(up, (void *) upath, 0);
  if (kpath == nil) {
    return nil;
  }

  path = strtopath(up->dot, kpath);

  return fileremove(path);
}

reg_t
sysmount(int outfd, int infd, const char *upath, uint32_t attr)
{
  struct bindingfid *fid;
  struct chan *in, *out;
  struct binding *b;
  struct path *path;
  const char *kpath;
  struct proc *p;
  int ret;

  kpath = kaddr(up, (void *) upath, 0);
  if (kpath == nil) {
    return ERR;
  }
  
  out = fdtochan(up->fgroup, outfd);
  if (out == nil) {
    return ERR;
  } else if (!(out->mode & O_WRONLY)) {
    return EMODE;
  }

  in = fdtochan(up->fgroup, infd);
  if (in == nil) {
    return ERR;
  } else if  (!(in->mode & O_RDONLY)) {
    return EMODE;
  }

  path = strtopath(up->dot, kpath);

  fid = findfile(path, &ret);
  if (ret != OK) {
    return ret;
  }
  
  b = bindingnew(out, in, attr);
  if (b == nil) {
    bindingfidfree(fid);
    pathfree(path);
    return ENOMEM;
  }

  p = procnew();
  if (p == nil) {
    bindingfidfree(fid);
    bindingfree(b);
    pathfree(path);
    return ENOMEM;
  }

  ret = ngroupaddbinding(up->ngroup, b, fid, b->fids);
  if (ret != OK) {
    procexit(p, 0);
    bindingfree(b);
    bindingfidfree(fid);
    pathfree(path);
    return ret;
  }

  forkfunc(p, &mountproc, (void *) b);
  b->srv = p;

  procready(p);

  return OK;
}

reg_t
sysbind(const char *old, const char *new)
{
  struct bindingfid *bo, *bn;
  struct path *po, *pn;
  int ret;

  if (kaddr(up, old, 0) == nil || kaddr(up, new, 0) == nil) {
    return ERR;
  }

  po = strtopath(up->dot, old);
  pn = strtopath(up->dot, new);

  bo = findfile(po, &ret);
  if (ret != OK) {
    pathfree(po);
    pathfree(pn);
    return ret;
  }

  bn = findfile(pn, &ret);
  if (ret != OK) {
    bindingfidfree(bo);
    pathfree(po);
    pathfree(pn);
    return ret;
  }

  ret = ngroupaddbinding(up->ngroup, bo->binding, bn, bo);
  if (ret != OK) {
    bindingfidfree(bo);
    bindingfidfree(bn);
    pathfree(po);
    pathfree(pn);
    return ret;
  }

  return OK;
}

reg_t
sysunbind(const char *upath)
{
  struct bindingfid *fid;
  struct path *path;
  int ret;
  
  if (kaddr(up, upath, 0) == nil) {
    return ERR;
  }

  path = strtopath(up->dot, upath);

  fid = findfile(path, &ret);
  if (ret != OK) {
    pathfree(path);
    return ret;
  }

  ret = ngroupremovebinding(up->ngroup, fid);
  
  bindingfidfree(fid);
  pathfree(path);

  return ret;
}

reg_t
sysdup(int old)
{
  struct chan *c;
  int new;

  c = fdtochan(up->fgroup, old);
  if (c == nil) {
    return ERR;
  }
  
  new = fgroupaddchan(up->fgroup, c);
  if (new < 0) {
    return new;
  }

  atomicinc(&c->refs);
  return new;
}

reg_t
sysdup2(int old, int new)
{
  struct chan *c;
  int r;
  
  c = fdtochan(up->fgroup, old);
  if (c == nil) {
    return ERR;
  }
  
  r = fgroupreplacechan(up->fgroup, c, new);
  if (r < 0) {
    return r;
  }

  atomicinc(&c->refs);
  return r;
}

reg_t
syscleanpath(char *opath, char *cpath, size_t cpathlen)
{
  struct path *rpath;
  char *spath;
  size_t l;

  if (kaddr(up, opath, 0) == nil) {
    return ERR;
  }
  
  rpath = strtopath(up->dot, opath);

  spath = pathtostr(rpath, &l);

  if (l + 1 > cpathlen) {
    free(spath);
    pathfree(rpath);
    return ERR;
  }

  memmove(cpath, spath, l);
  cpath[l] = 0;
  
  free(spath);
  pathfree(rpath);

  return l;
}
