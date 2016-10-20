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
#include <err.h>
#include <fs.h>
#include <fssrv.h>
#include <stdarg.h>
#include <string.h>

#include "fat.h"

static uint32_t
clusteroffset(struct fat *fat, uint32_t cluster);

static uint32_t
tableinfo(struct fat *fat, uint32_t prev);

static uint32_t
nextcluster(struct fat *fat, uint32_t prev);

static struct fat_dir_entry *
fatfindfileincluster(struct fat *fat, uint32_t sector, char *name);

static struct fat_dir_entry *
copyfileentryname(struct fat_dir_entry *start, char *name);


struct fat *
fatinit(int fd)
{
  struct fat_bs bs;
  struct fat *fat;

  fat = malloc(sizeof(struct fat));
  if (fat == nil) {
    printf("fat mount failed to alloc fat struct!\n");
    return nil;
  }

  fat->files = malloc(sizeof(struct fat_file));
  if (fat->files == nil) {
    printf("fat mount failed to alloc fat_file for root!\n");
    return nil;
  }

  fat->fd = fd;
  fat->nfid = ROOTFID;

  fat->files->fid = fat->nfid++;
  fat->files->opened = 0;
  fat->files->fatattr = 0;
  fat->files->attr = ATTR_wr|ATTR_rd|ATTR_dir;
  fat->files->size = 0;
  fat->files->next = nil;

  if (read(fat->fd, &bs, sizeof(bs)) != sizeof(bs)) {
    printf("fat mount failed to read boot sector.\n");
    return nil;
  }

  fat->bps = intcopylittle16(bs.bps);
  fat->spc = bs.spc;

  fat->nsectors = intcopylittle16(bs.sc16);
  if (fat->nsectors == 0) {
    fat->nsectors = intcopylittle32(bs.sc32);
  }

  fat->nft = bs.nft;

  fat->reserved = intcopylittle16(bs.res);

  fat->rde = intcopylittle16(bs.rde);
  if (fat->rde > 0) {
    fat->files->startcluster = 0;
    fat->files->size = fat->rde * 32;

    fat->spf = intcopylittle16(bs.spf);

    fat->rootdir = fat->reserved + fat->spf * fat->nft;
    fat->dataarea = fat->rootdir + (((fat->rde * 32) + (fat->bps - 1))
				  / fat->bps);
  } else {
    fat->spf = intcopylittle32(bs.ext.high.spf);

    fat->files->startcluster =
      intcopylittle32(bs.ext.high.rootcluster);

    fat->dataarea = fat->reserved + fat->spf * fat->nft;
  }

  fat->nclusters = (fat->nsectors - fat->dataarea) / fat->spc;

  if (fat->nclusters < 4085) {
    printf("FAT12 not supported\n");
    free(fat->files);
    free(fat);
    return nil;
  } else if (fat->nclusters < 65525) {
    fat->type = FAT16;
  } else {
    fat->type = FAT32;
  }

  fat->buf = malloc(fat->spc * fat->bps);
  if (fat->buf == nil) {
    return nil;
  }

  return fat;
}

struct fat_file *
fatfindfid(struct fat *fat, uint32_t fid)
{
  struct fat_file *file;

  for (file = fat->files; file != nil; file = file->next) {
    if (file->fid == fid) {
      return file;
    }
  }

  return nil;
}

void
fatclunkfid(struct fat *fat, uint32_t fid)
{
  struct fat_file *f, *p;

  p = nil;
  for (f = fat->files; f != nil; p = f, f = f->next) {
    if (f->fid == fid) {
      if (p == nil) {
	fat->files = f->next;
      } else {
	p->next = f->next;
      }

      break;
    }
  }

  if (f == nil) {
    return;
  }

  free(f);
}

uint32_t
clusteroffset(struct fat *fat, uint32_t cluster)
{
  return (fat->dataarea + ((cluster - 2) * fat->spc))
    * fat->bps;
}

uint32_t
nextcluster(struct fat *fat, uint32_t prev)
{
  uint32_t v;

  v = tableinfo(fat, prev);

  switch (fat->type) {
  case FAT16:
    v &= 0xffff;
    if (v >= 0xfff8) {
      v = 0;
    }
    break;
  case FAT32:
    v &= 0x0fffffff;
    if (v >= 0x0ffffff8) {
      v = 0;
    }
    break;
  }

  return v;
}

uint32_t
tableinfo(struct fat *fat, uint32_t cluster)
{
  uint32_t val = 0;
  size_t sec = 0, off;

  switch (fat->type) {
  case FAT16:
    sec = cluster * 16;
    break;
  case FAT32:
    sec = cluster * 32;
    break;
  }

  sec /= 8 * fat->bps;

  printf("seek to pos %i + res...\n", sec);

  if (seek(fat->fd, (fat->reserved + sec) * fat->bps, SEEK_SET) < 0) {
    printf("fat mount failed to seek.\n");
    return 0;
  }

  printf("read table sector\n");
  if (read(fat->fd, fat->buf, fat->bps) != fat->bps) {
    printf("fat mount failed to read table!\n");
    return 0;
  }

  off = cluster - sec * fat->bps;

  printf("offset from sector is %i\n", off);
 
  switch (fat->type) {
  case FAT16:
    val = intcopylittle16(fat->buf + off);
    break;
  case FAT32:
    val = intcopylittle32(fat->buf + off);
    break;
  }

  printf("so val is 0x%h\n", val);

  return val;
}
 
struct fat_dir_entry *
copyfileentryname(struct fat_dir_entry *file, char *name)
{
  struct fat_lfn *lfn;
  size_t i, j;

  if (file->attr == 0) {
    return nil;
  }

  while ((file->attr & FAT_ATTR_lfn) == FAT_ATTR_lfn) {
    printf("This is a long file name entry, not sure what to do\n");

    lfn = (struct fat_lfn *) file++;
    
    printf("order = %i\n", lfn->order);
  } 

  i = 0;
  for (j = 0; j < sizeof(file->name) && file->name[j] != ' '; j++)
    name[i++] = file->name[j];

  if (file->ext[0] != ' ') {
    name[i++] = '.';
  
    for (j = 0; j < sizeof(file->ext) && file->ext[j] != ' '; j++)
      name[i++] = file->ext[j];
  }

  name[i] = 0;

  return file + 1;
}

struct fat_dir_entry *
fatfindfileincluster(struct fat *fat, uint32_t sector, char *name)
{
  struct fat_dir_entry *file;
  char fname[NAMEMAX];

  if (seek(fat->fd, sector * fat->bps, SEEK_SET) < 0) {
    return nil;
  }

  if (read(fat->fd, fat->buf, fat->spc * fat->bps) < 0) {
    return nil;
  }
  
  file = (struct fat_dir_entry *) fat->buf;
  while (file != nil && (uint8_t *) file
	 < fat->buf + fat->spc * fat->bps) {

    file = copyfileentryname(file, fname);
    if (strncmp(fname, name, NAMEMAX)) {
      return file;
    }
  }

  return nil;
}

struct fat_file *
fatfilefind(struct fat *fat, struct fat_file *parent,
	    char *name, int *err)
{
  struct fat_dir_entry *direntry;
  struct fat_file *child;
  uint32_t cluster;

  direntry = nil;
  if (parent->startcluster == 0) {
    direntry = fatfindfileincluster(fat, fat->rootdir, name);
  } else {
    cluster = parent->startcluster;
    while (cluster != 0) {
      direntry = fatfindfileincluster(fat,
				      clusteroffset(fat, cluster),
				      name);
      if (direntry != nil) {
	break;
      } else {
	cluster = nextcluster(fat, cluster);
      }
    }
  }

  if (direntry == nil) {
    *err = ENOFILE;
    return nil;
  }

  child = malloc(sizeof(struct fat_file));
  if (child == nil) {
    *err = ENOMEM;
    return nil;
  }

  child->fid = fat->nfid++;
  child->opened = 0;
  child->fatattr = direntry->attr;

  strlcpy(child->name, name, NAMEMAX);

  child->attr = 0;

  if (child->fatattr & FAT_ATTR_read_only) {
    child->attr |= ATTR_rd;
  } else {
    child->attr |= ATTR_rd | ATTR_wr;
  }

  if (child->fatattr & FAT_ATTR_directory) {
    child->attr = ATTR_dir;
  }

  child->size = intcopylittle32(direntry->size);
  
  child->startcluster =
    ((uint32_t) intcopylittle16(direntry->cluster_high)) << 16;
  child->startcluster |=
    ((uint32_t) intcopylittle16(direntry->cluster_low));

  child->next = fat->files;
  fat->files = child;

  *err = OK;
  return child;
}


int
fatreadfile(struct fat *fat, struct fat_file *file,
	    uint8_t *buf, uint32_t offset, uint32_t len,
	    int *err)
{
  *err = ENOIMPL;
  return 0;
}

static uint32_t
readdirtobuf(struct fat *fat, uint8_t *buf,
	     uint32_t offset, uint32_t len)
{
  struct fat_dir_entry *entry;
  char name[NAMEMAX];
  uint32_t pos, rlen;
  uint8_t l;

  rlen = 0;
  pos = 0;
  entry = (struct fat_dir_entry *) fat->buf;

  while (entry != nil && (uint8_t *) entry
	 < fat->buf + fat->spc * fat->bps) {

    entry = copyfileentryname(entry, name);
    
    l = strlen(name) + 1;

    if (pos >= offset) {
      memmove(&buf[rlen], &l, sizeof(uint8_t));
      rlen += 1;
      if (rlen >= len) {
	break;
      }
    }

    pos += 1;

    if (pos >= offset) {
      memmove(&buf[rlen], name, l);
      rlen += l;
      if (rlen >= len) {
	break;
      }
    }

    pos += l;
  }

  return rlen;
}

int
fatreaddir(struct fat *fat, struct fat_file *file,
	   uint8_t *buf, uint32_t offset, uint32_t len,
	   int *err)
{
  uint32_t cluster, rlen;

  *err = OK;
  
  if (file->startcluster == 0) {

    if (seek(fat->fd, fat->rootdir * fat->bps, SEEK_SET) < 0) {
      *err = ERR;
      return 0;
    }

    if (read(fat->fd, fat->buf, fat->spc * fat->bps) < 0) {
      *err = ERR;
      return 0;
    }

    rlen = readdirtobuf(fat, buf, offset, len);
  } else {

    rlen = 0;
    cluster = file->startcluster;
    while (cluster != 0) {
      if (seek(fat->fd, clusteroffset(fat, cluster), SEEK_SET) < 0) {
	*err = ERR;
	return 0;
      }

      if (read(fat->fd, fat->buf, fat->spc * fat->bps) < 0) {
	*err = ERR;
	return 0;
      }

      if (offset > rlen) {
	offset -= rlen;
      } else {
	offset = 0;
      }

      rlen += readdirtobuf(fat, &buf[rlen], offset - rlen, len - rlen);
      if (rlen >= len) {
	break;
      }

      cluster = nextcluster(fat, cluster);
    }
  }
  
  return rlen;
}
