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
#include <block.h>

#define MBR_part_len 16
#define MBR_part1    0x1be
#define MBR_part2    0x1ce
#define MBR_part3    0x1de
#define MBR_part4    0x1ee

struct mbrpartition {
  uint8_t status;
  uint8_t start_chs[3];
  uint8_t type;
  uint8_t end_chs[3];
  uint32_t lba;
  uint32_t sectors;
} __attribute__ ((__packed__));

struct mbr {
  uint8_t bootstrap[446];
  struct mbrpartition parts[4];
  uint8_t bootsignature[2];
};

struct partition {
  bool active;
  
  uint8_t lname;
  uint8_t name[FS_LNAME_MAX+1];
  
  uint32_t fid;
  struct stat stat;

  uint32_t lba, sectors;

  struct mbrpartition *mbr;
};

static struct stat rootstat = {
  ATTR_rd|ATTR_wr|ATTR_dir,
  0,
};

static uint8_t *rootbuf;

static struct blkdevice *device;

static struct mbr mbr;
struct partition parts[5]; /* 4 mbr parts + raw */

static void
initparts(void)
{
  char c = 'a';
  int i;

  snprintf((char *) parts[0].name, FS_LNAME_MAX,
	   "%sraw", device->name);

  parts[0].lname = strlen((char *) parts[0].name);

  parts[0].fid = 1;
  parts[0].active = true;
  parts[0].stat.size = device->nblk * 512;
  parts[0].stat.attr = ATTR_wr|ATTR_rd;
  parts[0].lba = 0;
  parts[0].sectors = device->nblk;
  parts[0].mbr = nil;

  for (i = 1; i < 5; i++) {
    parts[i].fid = i + 1;

    snprintf((char *) parts[i].name, FS_LNAME_MAX,
	     "%s%c", device->name, c + i);

    parts[i].lname = strlen((char *) parts[i].name);

    parts[i].active = false;
  }
}

static void
updateparts(void)
{
  int i;
  uint8_t *buf;

  rootstat.size = 1 + parts[0].lname;

  for (i = 1; i < 5; i++) {
    parts[i].mbr = &mbr.parts[i-1];
   
    if (parts[i].mbr->type != 0) {
      parts[i].active = true;
      
      rootstat.size += sizeof(uint8_t)
	* (1 + parts[i].lname);

      memmove(&parts[i].lba, &mbr.parts[i].lba,
	      sizeof(uint32_t));
      memmove(&parts[i].sectors, &mbr.parts[i].sectors,
	      sizeof(uint32_t));

      parts[i].stat.size = parts[i].sectors * 512;

      parts[i].active = true;

      rootstat.size += 1 + parts[i].lname;
    } else {
      parts[i].active = false;
    }
  }

  if (rootbuf != nil) {
    free(rootbuf);
  }
  
  rootbuf = malloc(rootstat.size);
  if (rootbuf == nil) {
    printf("MBR failed to malloc rootbuf!\n");
    exit(-1);
  }

  buf = rootbuf;
  for (i = 0; i < 5; i++) {
    memmove(buf, &parts[i].lname, sizeof(uint8_t));
    buf += sizeof(uint8_t);
    memmove(buf, &parts[i].name, sizeof(uint8_t) * parts[i].lname);
    buf += sizeof(uint8_t) * parts[i].lname;
  }
}

static bool
updatembr(void)
{
  if (!(device->read(device->aux, 0, (uint8_t *) &mbr))) {
    printf("read failed\n");
    return false;
  }

  updateparts();

  return true;
}

static struct partition *
fidtopart(uint32_t fid)
{
  int i;

  for (i = 0; i < 5; i++) {
    if (parts[i].fid == fid) {
      if (parts[i].active) {
	return &parts[i];
      } else {
	return nil;
      }
    }
  }

  return nil;
}

static void
bfid(struct request *req, struct response *resp)
{
  int i;

  for (i = 0; i < 5; i++) {
    if (parts[i].active) {
      if (parts[i].lname != req->lbuf) continue;
      if (strncmp((char *) parts[i].name, (char *) req->buf,
		  parts[i].lname)) {
	break;
      }
    }
  }

  if (i == 4) {
    resp->ret = ENOFILE;
    return;
  }

  resp->buf = malloc(sizeof(uint32_t));
  if (resp->buf == nil) {
    resp->ret = ENOMEM;
    return;
  }

  resp->lbuf = sizeof(uint32_t);
  memmove(resp->buf, &parts[i].fid, sizeof(uint32_t));
  resp->ret = OK;
}

static void
bstat(struct request *req, struct response *resp)
{
  struct partition *part;
  struct stat *s = nil;
  
  if (req->fid == 0) {
    s = &rootstat;
  }

  part = fidtopart(req->fid);
  if (part != nil) {
    s = &(part->stat);
  }

  if (s == nil) {
    resp->ret = ENOFILE;
  } else {
    resp->buf = (uint8_t *) s;
    resp->lbuf = sizeof(struct stat);
    resp->ret = OK;
  }
}

static void
bopen(struct request *req, struct response *resp)
{
  printf("%s opened\n", device->name);
  resp->ret = OK;
}

static void
bclose(struct request *req, struct response *resp)
{
  printf("%s close\n", device->name);
  resp->ret = OK;
}

static void
breadroot(struct request *req, struct response *resp,
	  uint32_t offset, uint32_t len)
{
  uint32_t rlen;

  rlen = offset + len > rootstat.size ? rootstat.size - offset : len;
  
  resp->buf = malloc(rlen);
  if (resp->buf == nil) {
    resp->ret = ENOMEM;
    return;
  }

  memmove(resp->buf, rootbuf + offset, rlen);
  resp->lbuf = rlen;
  resp->ret = OK;
}

static void
breadblocks(struct request *req, struct response *resp,
	    struct partition *part,
	    uint32_t blk, uint32_t nblk)
{
  uint8_t *buf;
  uint32_t i;

  resp->buf = malloc(nblk * 512);
  if (resp->buf == nil) {
    resp->ret = ENOMEM;
    return;
  }

  buf = resp->buf;

  for (i = 0; i < nblk && part->lba + blk + i < part->sectors; i++) {
    if (!device->read(device->aux, part->lba + blk + i, buf)) {
      free(resp->buf);
      resp->ret = ERR;
      return;
    }
    
    buf += 512;
  }

  resp->lbuf = i * 512;
  resp->ret = OK;
}

static void
bread(struct request *req, struct response *resp)
{
  struct partition *part;
  uint32_t offset, len;
  uint8_t *buf;

  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(&len, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  
  if (req->fid == 0) {
    breadroot(req, resp, offset, len);
    return;
  } else if (len % 512 != 0 || offset % 512 != 0) {
    resp->ret = ENOIMPL;
    return;
  } else {
    part = fidtopart(req->fid);
    if (part == nil) {
      resp->ret = ENOFILE;
    } else {
      breadblocks(req, resp, part, offset / 512, len / 512);
    }
  }
}

static void
bwrite(struct request *req, struct response *resp)
{
  resp->ret = ENOIMPL;
}

static struct fsmount mount = {
  &bfid,
  &bstat,
  &bopen,
  &bclose,
  nil,
  nil,
  &bread,
  &bwrite,
  nil,
};

int
mbrmountthread(struct blkdevice *d, uint8_t *dir)
{
  int p1[2], p2[3], fd, i;
  char filename[1024];

  device = d;

  i = fork(FORK_smem|FORK_sngroup|FORK_sfgroup);
  if (i != 0) {
    return i;
  }

  initparts();
  
  if (!updatembr()) {
    printf("%s failed to read mbr\n", device->name);
    exit(-2);
  }

  snprintf(filename, sizeof(filename), "%s/%s", dir, device->name);

  fd = open(filename, O_WRONLY|O_CREATE, ATTR_dir|ATTR_rd);
  if (fd < 0) {
    printf("%s failed to create %s.\n", device->name, filename);
    return -1;
  }

  if (pipe(p1) == ERR) {
    return -2;
  } else if (pipe(p2) == ERR) {
    return -2;
  }

  if (bind(p1[1], p2[0], filename) == ERR) {
    return -3;
  }

  close(p1[1]);
  close(p2[0]);

  printf("mounting %s on %s\n", device->name, filename);
  return fsmountloop(p1[0], p2[1], &mount);
}

