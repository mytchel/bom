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

static uint32_t nfid = ROOTFID;

struct fat *fat;

static void
bfid(struct request *req, struct response *resp)
{
  struct fat_file *new, *parent;
  struct response_fid *fidresp;
  char *name;

  name = (char *) req->buf;

  printf("fat mount find '%s' in %i and give fid %i\n",
	 name, req->fid, nfid++);

  parent = fatfilefindfid(fat, req->fid);
  if (parent == nil) {
    resp->ret = ENOFILE;
    return;
  }

  new = fatfilefind(fat, parent, name, &resp->ret);
  if (new == nil) {
    resp->ret = ENOFILE;
    return;
  }

  fidresp = malloc(sizeof(struct response_fid));
  if (fidresp == nil) {
    resp->ret = ENOMEM;
    return;
  }

  fidresp->fid = new->fid;
  fidresp->attr = new->attr;

  resp->buf = (uint8_t *) fidresp;
  resp->lbuf = sizeof(struct response_fid);
  resp->ret = OK;
}

static void
bclunk(struct request *req, struct response *resp)
{
  printf("fat mount should clunk %i\n", req->fid);
  resp->ret = ENOIMPL;
}

static void
bstat(struct request *req, struct response *resp)
{
  resp->ret = ENOIMPL;
}

static void
bopen(struct request *req, struct response *resp)
{
  resp->ret = OK;
}

static void
bclose(struct request *req, struct response *resp)
{
  resp->ret = OK;
}

static void
bread(struct request *req, struct response *resp)
{
  struct request_read *rr;

  rr = (struct request_read *) req->buf;

  printf("fat mount should read %i from %i len %i\n",
	 req->fid, rr->offset, rr->len);

  resp->ret = ENOIMPL;
}

static void
bwrite(struct request *req, struct response *resp)
{
  struct request_write *rw;

  rw = (struct request_write *) req->buf;

  printf("fat mount should write %i from %i len %i\n",
	 req->fid, rw->offset, rw->len);

  resp->ret = ENOIMPL;
}

static void
bcreate(struct request *req, struct response *resp)
{
  struct request_create *rc;

  rc = (struct request_create *) req->buf;

  printf("fat mount should create in %i file with attr 0b%b, name '%s'\n",
	 req->fid, rc->attr, rc->name);

  resp->ret = ENOIMPL;
}

static void
bremove(struct request *req, struct response *resp)
{
  printf("fat mount should remove %i\n", req->fid);
  resp->ret = ENOIMPL;
}

static struct fsmount mount = {
  .fid = &bfid,
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

