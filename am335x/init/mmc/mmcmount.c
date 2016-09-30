 /*
 *   Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <libc.h>
#include <fs.h>
#include <stdarg.h>
#include <string.h>

#include "mmc.h"
 
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
  uint32_t offset, len, blk, nblk, i;
  uint8_t *buf;

  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(&len, buf, sizeof(uint32_t));

  printf("%s read  from %i len %i\n", mmc.name, offset, len);
  if (len % 512 != 0) {
    resp->ret = ENOIMPL;
    return;
  }
  
  blk = offset / 512;
  nblk = len / 512;

  resp->buf = malloc(nblk * 512);
  if (resp->buf == nil) {
    resp->ret = ENOMEM;
    return;
  }

  buf = resp->buf;

  for (i = 0; i < nblk; i++) {
    readblock(blk + i, buf);
    buf += 512;
  }

  resp->lbuf = i * 512;
  resp->ret = OK;
}

static void
bwrite(struct request *req, struct response *resp)
{
  uint32_t offset, len, blk, nblk, i;
  uint8_t *buf;

  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  len = req->lbuf - sizeof(uint32_t);

  if (len % 512 != 0) {
    resp->ret = ENOIMPL;
    return;
  }
  
  blk = offset / 512;
  nblk = len / 512;

  resp->buf = malloc(sizeof(uint32_t));
  if (resp->buf == nil) {
    resp->ret = ENOMEM;
    return;
  }

  for (i = 0; i < nblk; i++) {
    writeblock(blk + i, buf);
    buf += 512;
  }

  *(resp->buf) = i * 512;
  resp->ret = OK;
}

static void
bremove(struct request *req, struct response *resp)
{
  printf("%s remove, should exit.\n", mmc.name);
  resp->ret = ENOIMPL;
}

static struct fsmount mount = {
  &bopen,
  &bclose,
  nil,
  &bread,
  &bwrite,
  &bremove,
  nil,
};

int
mmcmount(char *filename)
{
  int fd, p1[2], p2[2];
  
  printf("%s binding to %s\n", mmc.name, filename);
  
  fd = open(filename, O_WRONLY|O_CREATE, ATTR_wr|ATTR_rd);
  if (fd < 0) {
    printf("%s failed to create %s.\n", mmc.name, filename);
    return -6;
  }

  if (pipe(p1) == ERR) {
    return -4;
  } else if (pipe(p2) == ERR) {
    return -5;
  }

  if (bind(p1[1], p2[0], filename) == ERR) {
    return -7;
  }

  close(p1[1]);
  close(p2[0]);

  mmc.mbrpid = fork(FORK_sngroup);
  if (mmc.mbrpid == 0) {
    close(p1[0]);
    close(p2[1]);

    rmmem((void *) regs, regsize);

    return mbrmount(filename, mmc.name);
  }
  return fsmountloop(p1[0], p2[1], &mount);
}
