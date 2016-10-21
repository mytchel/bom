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

#include <libc.h>
#include <mem.h>
#include <fs.h>
#include <fssrv.h>
#include <stdarg.h>
#include <string.h>

#include "fat.h"

struct fat *fat;

static void
bgetfid(struct request_getfid *req, struct response_getfid *resp)
{
  struct fat_file *new, *parent;

  parent = fatfindfid(fat, req->head.fid);
  if (parent == nil) {
    resp->head.ret = ENOFILE;
    return;
  }

  new = fatfilefind(fat, parent, req->body.name, &resp->head.ret);
  if (new == nil) {
    return;
  }

  resp->body.fid = new->fid;
  resp->body.attr = new->attr;
  resp->head.ret = OK;
}

static void
bclunk(struct request_clunk *req, struct response_clunk *resp)
{
  fatclunkfid(fat, req->head.fid);
  resp->head.ret = OK;
}

static void
bstat(struct request_stat *req, struct response_stat *resp)
{
  struct fat_file *f;

  f = fatfindfid(fat, req->head.fid);
  if (f == nil) {
    resp->head.ret = ENOFILE;
    return;
  }

  resp->body.stat.attr = f->attr;
  resp->body.stat.size = f->size;
  resp->head.ret = OK;
}

static void
bopen(struct request_open *req, struct response_open *resp)
{
  resp->head.ret = OK;
}

static void
bclose(struct request_close *req, struct response_close *resp)
{
  resp->head.ret = OK;
}

static void
bread(struct request_read *req, struct response_read *resp)
{
  struct fat_file *f;
  uint32_t offset, len;

  offset = req->body.offset;
  len = req->body.len;

  f = fatfindfid(fat, req->head.fid);
  if (f == nil) {
    resp->head.ret = ENOFILE;
    return;
  }

  if (f->attr & ATTR_dir) {
    resp->body.len = fatreaddir(fat, f, resp->body.data,
				offset, len, &resp->head.ret);
  } else {
    resp->body.len = fatreadfile(fat, f, resp->body.data,
				 offset, len, &resp->head.ret);
  }
}

static void
bwrite(struct request_write *req, struct response_write *resp)
{
  resp->head.ret = ENOIMPL;
}

static void
bcreate(struct request_create *req, struct response_create *resp)
{
  printf("fat mount should create in %i file with attr 0b%b, name '%s'\n",
	 req->head.fid, req->body.attr, req->body.name);

  resp->head.ret = ENOIMPL;
}

static void
bremove(struct request_remove *req, struct response_remove *resp)
{
  printf("fat mount should remove %i\n", req->head.fid);
  resp->head.ret = ENOIMPL;
}

static struct fsmount mount = {
  .getfid = &bgetfid,
  .clunk  = &bclunk,
  .stat = &bstat,
  .open = &bopen,
  .close = &bclose,
  .create = &bcreate,
  .remove = &bremove,
  .read = &bread,
  .write = &bwrite,
};

int
mountfat(char *device, char *dir)
{
  int p1[2], p2[3], fddev, fddir, i;

  fddev = open(device, O_RDWR);
  if (fddev < 0) {
    printf("failed to open device %s\n", device);
    return fddev;
  }

  fat = fatinit(fddev);
  if (fat == nil) {
    printf("fat init failed on %s\n", device);
    return ERR;
  }
  
  fddir = open(dir, O_RDONLY);
  if (fddir < 0) {
    printf("failed to open mount dir %s\n", dir);
    return fddev;
  }

  if (pipe(p1) == ERR) {
    printf("pipe failed\n");
    return ERR;
  } else if (pipe(p2) == ERR) {
    printf("pipe failed\n");
    return ERR;
  }

  if (bind(p1[1], p2[0], dir) == ERR) {
    printf("failed to bind to %s\n", dir);
    return ERR;
  }

  close(p1[1]);
  close(p2[0]);

  i = fork(FORK_sngroup);
  if (i != 0) {
    close(p1[0]);
    close(p2[1]);
    close(fddev);
    close(fddir);
    return i;
  }

  i = fsmountloop(p1[0], p2[1], &mount);
  exit(i);

  return 0;
}

