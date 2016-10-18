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
} __attribute__((__packed__));

struct mbr {
  uint8_t bootstrap[446];
  struct mbrpartition parts[4];
  uint8_t bootsignature[2];
};

struct partition {
  bool active;
  
  uint8_t lname;
  char name[NAMEMAX];
  
  uint32_t fid;
  struct stat stat;

  uint32_t lba, sectors;

  struct mbrpartition *mbr;
};

static struct stat rootstat = {
  ATTR_rd|ATTR_dir,
  0,
};

static uint8_t *rootbuf = nil;

static uint32_t nfid = ROOTFID;

static struct blkdevice *device = nil;

static struct mbr mbr;
struct partition parts[4], raw;

static void
initpart(struct partition *p, char *name)
{
  p->lname = strlcpy(p->name, name, NAMEMAX);
  p->fid = ++nfid;
  p->stat.attr = ATTR_wr|ATTR_rd;
}

static void
initparts(void)
{
  char name[NAMEMAX] = "a";
  int i;

  for (i = 0; i < 4; i++) {
    initpart(&parts[i], name);
    name[0]++;

    parts[i].mbr = &mbr.parts[i];
    parts[i].active = false;
  }

  initpart(&raw, "raw");

  raw.stat.size = device->nblk * 512;
  raw.active = true;
  raw.lba = 0;
  raw.sectors = device->nblk;
  raw.mbr = nil;
}

static void
updatembr(void)
{
  int i;
  uint8_t *buf;

  if (!(device->read(device->aux, 0, (uint8_t *) &mbr))) {
    printf("mbr read failed\n");
    exit(ERR);
  }

  rootstat.size = 1 + raw.lname;

  for (i = 0; i < 4; i++) {
    if (parts[i].mbr->type != 0) {
      parts[i].active = true;

      rootstat.size += 1 + parts[i].lname;

      memmove(&parts[i].lba, &parts[i].mbr->lba,
	      sizeof(uint32_t));

      memmove(&parts[i].sectors, &parts[i].mbr->sectors,
	      sizeof(uint32_t));

      parts[i].stat.size = parts[i].sectors * 512;
    } else {
      parts[i].active = false;
    }
  }

  if (rootbuf != nil) {
    free(rootbuf);
  }
  
  rootbuf = malloc(rootstat.size);
  if (rootbuf == nil) {
    printf("Failed to malloc mbr rootbuf!\n");
    exit(-1);
  }

  buf = rootbuf;

  memmove(buf, &raw.lname, sizeof(uint8_t));
  buf += sizeof(uint8_t);
  memmove(buf, raw.name, sizeof(uint8_t) * raw.lname);
  buf += sizeof(uint8_t) * raw.lname;

  for (i = 0; i < 4; i++) {
    if (parts[i].active == false) continue;

    memmove(buf, &parts[i].lname, sizeof(uint8_t));
    buf += sizeof(uint8_t);
    memmove(buf, parts[i].name, parts[i].lname);
    buf += parts[i].lname;
  }
}

static struct partition *
fidtopart(uint32_t fid)
{
  int i;

  if (fid == raw.fid) {
    return &raw;
  }
  
  for (i = 0; i < 4; i++) {
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
  struct partition *part = nil;
  struct response_fid *respfid;
  int i;

  if (strncmp(raw.name, (char *) req->buf, NAMEMAX)) {
    part = &raw;
  } else {
    for (i = 0; i < 4; i++) {
      if (parts[i].active) {
	if (strncmp(parts[i].name, (char *) req->buf, NAMEMAX)) {
	  part = &parts[i];
	  break;
	}
      }
    }
  } 

  if (part == nil) {
    resp->ret = ENOFILE;
    return;
  }

  respfid = malloc(sizeof(struct response_fid));
  if (respfid == nil) {
    resp->ret = ENOMEM;
    return;
  }

  respfid->fid = part->fid;
  respfid->attr = part->stat.attr;
  
  resp->buf = (uint8_t *) respfid;
  resp->lbuf = sizeof(struct response_fid);
  resp->ret = OK;
}

static void
bclunk(struct request *req, struct response *resp)
{
  printf("mbr %s clunk %i\n", device->name, req->fid);
  
  resp->ret = OK;
}

static void
bstat(struct request *req, struct response *resp)
{
  struct partition *part;
  struct stat *s = nil;

  if (req->fid == ROOTFID) {
    s = &rootstat;
  } else {
    part = fidtopart(req->fid);
    if (part != nil) {
      s = &(part->stat);
    }
  }

  if (s == nil) {
    resp->ret = ENOFILE;
  } else {
    resp->buf = malloc(sizeof(struct stat));
    if (resp->buf == nil) {
      resp->ret = ENOMEM;
    } else {
      memmove(resp->buf, s, sizeof(struct stat));
      resp->lbuf = sizeof(struct stat);
      resp->ret = OK;
    }
  }
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
breadroot(struct request *req, struct response *resp,
	  uint32_t offset, uint32_t len)
{
  uint32_t rlen;

  if (offset + len > rootstat.size) {
    rlen = rootstat.size - offset;
  } else {
    rlen = len;
  }

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
    printf("%s mbr read sector %i\n", device->name, part->lba + blk + i);
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
  struct request_read *rr;
  struct partition *part;

  rr = (struct request_read *) req->buf;

  if (req->fid == ROOTFID) {
    breadroot(req, resp, rr->offset, rr->len);
    return;
  } else if (rr->len % 512 != 0 || rr->offset % 512 != 0) {
    resp->ret = ENOIMPL;
    return;
  } else {
    part = fidtopart(req->fid);
    if (part == nil) {
      resp->ret = ENOFILE;
    } else {
      breadblocks(req, resp, part,
		  rr->offset / 512, rr->len / 512);
    }
  }
}

static void
bwrite(struct request *req, struct response *resp)
{
  resp->ret = ENOIMPL;
}

static struct fsmount mount = {
  .fid = &bfid,
  .clunk = &bclunk,
  .stat = &bstat,
  .open = &bopen,
  .close = &bclose,
  .read = &bread,
  .write = &bwrite,
};

int
mbrmount(struct blkdevice *d, uint8_t *dir)
{
  int p1[2], p2[3], fd;
  char filename[1024];

  device = d;

  initparts();
  updatembr();

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

  return fsmountloop(p1[0], p2[1], &mount);
}

