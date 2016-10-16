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
#include <fs.h>
#include <fssrv.h>
#include <stdarg.h>
#include <string.h>

#include "fat.h"

static uint32_t nfid = ROOTFID;

static void
bfid(struct request *req, struct response *resp)
{
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
  uint32_t offset, len;
  uint8_t *buf;

  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(&len, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);

  resp->ret = ENOIMPL;
}

static void
bwrite(struct request *req, struct response *resp)
{
  resp->ret = ENOIMPL;
}

static void
bcreate(struct request *req, struct response *resp)
{
  nfid++;
  resp->ret = ENOIMPL;
}

static void
bremove(struct request *req, struct response *resp)
{
  resp->ret = ENOIMPL;
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
mountfat(char *device, char *dir)
{
  int p1[2], p2[3], fddev, fddir, i;

  fddev = open(device, O_RDWR);
  if (fddev < 0) {
    printf("failed to open device %s\n", device);
    return fddev;
  }

  i = fatinit(fddev);
  if (i < 0) {
    printf("fat init failed on %s\n", device);
    return i;
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

