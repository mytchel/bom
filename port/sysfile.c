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

/* Need to fix syscalls with path arguments so check if the
 * path is a valid address in the processes memory.
 */

reg_t
syschdir(va_list args)
{
  const char *upath, *kpath;
  struct path *path;
  struct chan *c;
  int err;

  upath = va_arg(args, const char *);

  kpath = kaddr(current, (void *) upath, 0);
  if (kpath == nil) {
    return ERR;
  }

  path = realpath(current->dot, kpath);

  c = fileopen(path, O_RDONLY|O_DIR, 0, &err);
  if (err != OK) {
    freepath(path);
    return err;
  }

  freepath(current->dot);

  if (current->dotchan != nil) {
    freechan(current->dotchan);
  }

  current->dotchan = c;
  current->dot = path;

  return OK;
}

int
read(int fd, void *buf, size_t len)
{
  struct chan *c;
  int r;

  c = fdtochan(current->fgroup, fd);
  if (c == nil) {
    return ERR;
  } else if (!(c->mode & O_RDONLY)) {
    return EMODE;
  } else if (len == 0) {
    return ERR;
  }

  lock(&c->lock);
  r = chantypes[c->type]->read(c, buf, len);
  unlock(&c->lock);

  return r;
}

reg_t
sysread(va_list args)
{
  int fd;
  size_t len;
  void *buf, *kbuf;

  fd = va_arg(args, int);
  buf = va_arg(args, void *);
  len = va_arg(args, size_t);

  kbuf = kaddr(current, buf, len);

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

  c = fdtochan(current->fgroup, fd);
  if (c == nil) {
    return ERR;
  } else if (!(c->mode & O_WRONLY)) {
    return EMODE;
  } else if (len == 0) {
    return ERR;
  }
	
  lock(&c->lock);
  r = chantypes[c->type]->write(c, buf, len);
  unlock(&c->lock);

  return r;
}

reg_t
syswrite(va_list args)
{
  int fd;
  size_t len;
  void *buf, *kbuf;
	
  fd = va_arg(args, int);
  buf = va_arg(args, void *);
  len = va_arg(args, size_t);

  kbuf = kaddr(current, buf, len);

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

  c = fdtochan(current->fgroup, fd);
  if (c == nil) {
    return ERR;
  }
	
  lock(&c->lock);
  r = chantypes[c->type]->seek(c, offset, whence);
  unlock(&c->lock);
	
  return r;
}

reg_t
sysseek(va_list args)
{
  int fd, whence;
  size_t offset;
	
  fd = va_arg(args, int);
  offset = va_arg(args, size_t);
  whence = va_arg(args, int);

  return seek(fd, offset, whence);
}
     
reg_t
sysstat(va_list args)
{
  const char *upath, *kpath;
  struct stat *ustat, *stat;
  struct path *path;
	
  upath = va_arg(args, const char *);
  ustat = va_arg(args, struct stat *);

  kpath = kaddr(current, (void *) upath, 0);
  
  stat = (struct stat *) kaddr(current, (void *) ustat,
			       sizeof(struct stat *));

  if (stat == nil || kpath == nil) {
    return ERR;
  }
  
  path = realpath(current->dot, kpath);

  return filestat(path, stat);
}

reg_t
sysclose(va_list args)
{
  int fd;
  struct chan *c;

  fd = va_arg(args, int);

  c = fdtochan(current->fgroup, fd);
  if (c == nil) {
    return ERR;
  }

  /* Remove fd. */
  lock(&current->fgroup->lock);
  current->fgroup->chans[fd] = nil;
  unlock(&current->fgroup->lock);

  freechan(c);
  
  return OK;
}

reg_t
sysopen(va_list args)
{
  int err;
  uint32_t mode, cmode;
  const char *upath, *kpath;
  struct chan *c;
  struct path *path;
	
  upath = va_arg(args, const char *);
  mode = va_arg(args, uint32_t);

  kpath = kaddr(current, (void *) upath, 0);
  if (kpath == nil) {
    return ERR;
  }
  
  if (mode & O_CREATE) {
    cmode = va_arg(args, uint32_t);
  } else {
    cmode = 0;
  }

  path = realpath(current->dot, kpath);

  c = fileopen(path, mode, cmode, &err);

  if (c == nil) {
    freepath(path);
    return err;
  } else {
    return addchan(current->fgroup, c);
  }
}

reg_t
syspipe(va_list args)
{
  int *fds;
  struct chan *c0, *c1;
	
  fds = va_arg(args, int*);

  if (!newpipe(&c0, &c1)) {
    return ENOMEM;
  }
	
  fds[0] = addchan(current->fgroup, c0);
  fds[1] = addchan(current->fgroup, c1);
	
  return OK;
}

reg_t
sysremove(va_list args)
{
  const char *upath, *kpath;
  struct path *path;
	
  upath = va_arg(args, const char *);

  kpath = kaddr(current, (void *) upath, 0);
  if (kpath == nil) {
    return nil;
  }

  path = realpath(current->dot, kpath);

  return fileremove(path);
}

reg_t
sysbind(va_list args)
{
  int infd, outfd;
  const char *upath, *kpath;
  struct proc *p;
  struct path *path;
  struct chan *in, *out;
  struct binding_list *bl;
	
  outfd = va_arg(args, int);
  infd = va_arg(args, int);
  upath = va_arg(args, const char *);

  kpath = kaddr(current, (void *) upath, 0);
  if (kpath == nil) {
    return ERR;
  }
  
  out = fdtochan(current->fgroup, outfd);
  if (out == nil) {
    return ERR;
  } else if (!(out->mode & O_WRONLY)) {
    return EMODE;
  }

  in = fdtochan(current->fgroup, infd);
  if (in == nil) {
    return ERR;
  } else if  (!(in->mode & O_RDONLY)) {
    return EMODE;
  }
	
  lock(&current->ngroup->lock);

  path = realpath(current->dot, kpath);
 
  bl = malloc(sizeof(struct binding_list));
  if (bl == nil) {
    freepath(path);
    unlock(&current->ngroup->lock);
    return ENOMEM;
  }
		
  bl->binding = newbinding(path, out, in);
  if (bl->binding == nil) {
    free(bl);
    freepath(path);
    unlock(&current->ngroup->lock);
    return ENOMEM;
  }

  p = newproc(30);
  if (p == nil) {
    freebinding(bl->binding);
    free(bl);
    unlock(&current->ngroup->lock);
    return ENOMEM;
  }

  bl->next = current->ngroup->bindings;
  current->ngroup->bindings = bl;
	
  forkfunc(p, &mountproc, (void *) bl->binding);
	
  bl->binding->srv = p;

  disableintr();
  procready(p);
  enableintr();
	
  unlock(&current->ngroup->lock);

#if DEBUG == 1
  char *str = (char *) pathtostr(path, nil);
  printf("Binding %i to '%s', kproc %i\n", current->pid, kpath,
	 p->pid);
  free(str);
#endif
 
  return OK;
}

reg_t
syscleanpath(va_list args)
{
  char *opath, *cpath, *spath;
  size_t cpathlen, l;
  struct path *rpath;

  opath = va_arg(args, char *);
  cpath = va_arg(args, char *);
  cpathlen = va_arg(args, size_t);

  rpath = realpath(current->dot, opath);

  spath = pathtostr(rpath, &l);

  if (l + 1 > cpathlen) {
    free(spath);
    freepath(rpath);
    return ERR;
  }

  memmove(cpath, spath, l);
  cpath[l] = 0;
  
  free(spath);
  freepath(rpath);

  return l;
}
