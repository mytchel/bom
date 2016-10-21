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

static bool
readsectors(struct fat *fat, uint32_t sector);

static uint32_t
clustertosector(struct fat *fat, uint32_t cluster);

static uint32_t
tableinfo(struct fat *fat, uint32_t prev);

static uint32_t
nextcluster(struct fat *fat, uint32_t prev);

static struct fat_dir_entry *
fatfindfileincluster(struct fat *fat, uint32_t sector, char *name);

static struct fat_dir_entry *
copyfileentryname(struct fat_dir_entry *start, char *name);

static uint8_t *
builddirbufsectors(struct fat *fat, uint32_t sector,
		   uint8_t *buf, size_t *len, size_t *mlen);

static uint8_t *
builddirbuf(struct fat *fat, uint32_t cluster, size_t *len);


struct fat *
fatinit(int fd)
{
  struct fat_bs bs;
  struct fat *fat;
  size_t mlen;

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
  fat->files->attr = ATTR_wr|ATTR_rd|ATTR_dir;
  fat->files->next = nil;

  if (read(fat->fd, &bs, sizeof(bs)) != sizeof(bs)) {
    printf("fat mount failed to read boot sector.\n");
    return nil;
  }

  fat->bps = intcopylittle16(bs.bps);
  fat->spc = bs.spc;

  fat->buf = malloc(fat->spc * fat->bps);
  if (fat->buf == nil) {
    return nil;
  }

  fat->nsectors = intcopylittle16(bs.sc16);
  if (fat->nsectors == 0) {
    fat->nsectors = intcopylittle32(bs.sc32);
  }

  fat->nft = bs.nft;

  fat->reserved = intcopylittle16(bs.res);

  fat->rde = intcopylittle16(bs.rde);
  if (fat->rde != 0) {
    fat->type = FAT16;

    fat->spf = intcopylittle16(bs.spf);

    fat->rootdir = fat->reserved + fat->nft * fat->spf;
    fat->dataarea = fat->rootdir +
      (((fat->rde * sizeof(struct fat_dir_entry)) + (fat->bps - 1))
       / fat->bps);

    fat->files->size = fat->rde * sizeof(struct fat_dir_entry);
    fat->files->startcluster = 0;

    mlen = 1;
    fat->files->dirbuflen = 0;
    fat->files->dirbuf =
      builddirbufsectors(fat, fat->rootdir,
		   nil, &fat->files->dirbuflen, &mlen);
  } else {
    fat->type = FAT32;

    fat->spf = intcopylittle32(bs.ext.high.spf);

    /* This should work for now */
    fat->files->size = fat->spc * fat->bps;

    fat->dataarea = fat->reserved + fat->spf * fat->nft;

    fat->files->startcluster =
      intcopylittle32(bs.ext.high.rootcluster);

    fat->files->dirbuf =
      builddirbuf(fat, fat->files->startcluster,
		  &fat->files->dirbuflen);
  }

  if (fat->files->dirbuf == nil) {
    printf("fat mount failed to read root dir\n");
    return nil;
  }

  fat->nclusters = (fat->nsectors - fat->dataarea) / fat->spc;

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

  if (f->dirbuf != nil) {
    free(f->dirbuf);
  }
    
  free(f);
}

uint32_t
clustertosector(struct fat *fat, uint32_t cluster)
{
  return (fat->dataarea + ((cluster - 2) * fat->spc));
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
  uint32_t sec, off = 0;

  switch (fat->type) {
  case FAT16:
    off = cluster * sizeof(uint16_t);
    break;
  case FAT32:
    off = cluster * sizeof(uint32_t);
    break;
  }

  sec = off / fat->bps;

  if (seek(fat->fd, (fat->reserved + sec) * fat->bps, SEEK_SET) < 0) {
    printf("fat mount failed to seek.\n");
    return 0;
  }

  if (read(fat->fd, fat->buf, fat->bps) != fat->bps) {
    printf("fat mount failed to read table!\n");
    return 0;
  }

  off = off % fat->bps;

  switch (fat->type) {
  case FAT16:
    val = intcopylittle16(fat->buf + off);
    break;
  case FAT32:
    val = intcopylittle32(fat->buf + off);
    break;
  }

  return val;
}

bool
readsectors(struct fat *fat, uint32_t sector)
{
  int i;
  
  if (seek(fat->fd, sector * fat->bps, SEEK_SET) < 0) {
    printf("failed to seek to sector %i\n", sector);
    return false;
  }

  for (i = 0; i < fat->spc; i++) {
    if (read(fat->fd, fat->buf + i * fat->bps, fat->bps) < 0) {
      printf("failed to read sector %i\n", sector + i);
      return false;
    }
  }

  return true;
}

struct fat_dir_entry *
copyfileentryname(struct fat_dir_entry *file, char *name)
{
  struct fat_lfn *lfn;
  size_t i, j;

  if (file->name[0] == 0) {
    return nil;
  }

  while ((file->attr & FAT_ATTR_lfn) == FAT_ATTR_lfn) {
    printf("This is a long file name entry, not sure what to do\n");

    lfn = (struct fat_lfn *) file++;
    
    printf("order = %i\n", lfn->order);
  } 

  i = 0;

  for (j = 0; j < sizeof(file->name) && file->name[j] != ' '; j++) {
    name[i++] = file->name[j];
  }

  if (file->ext[0] != ' ') {
    name[i++] = '.';
  
    for (j = 0; j < sizeof(file->ext) && file->ext[j] != ' '; j++) {
      name[i++] = file->ext[j];
    }
  }

  name[i] = 0;

  if (strcmp(name, ".") || strcmp(name, "..")) {
    return copyfileentryname(file + 1, name);
  } else {
    return file + 1;
  }
}

struct fat_dir_entry *
fatfindfileincluster(struct fat *fat, uint32_t sector, char *name)
{
  struct fat_dir_entry *file, *nfile;
  char fname[NAMEMAX];

  if (!readsectors(fat, sector)) {
    return nil;
  }
 
  file = (struct fat_dir_entry *) fat->buf;
  while ((uint8_t *) file < fat->buf + fat->spc * fat->bps) {

    nfile = copyfileentryname(file, fname);
    if (nfile == nil) {
      return nil;
    } else if (strncmp(fname, name, NAMEMAX)) {
      return file;
    } else {
      file = nfile;
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
  uint8_t attr;

  direntry = nil;

  if (parent->startcluster == 0) {
    direntry = fatfindfileincluster(fat, fat->rootdir, name);
  } else {
    cluster = parent->startcluster;
    while (cluster != 0) {
      direntry =
	fatfindfileincluster(fat, clustertosector(fat, cluster), name);

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
  child->dirbuf = nil;
  
  strlcpy(child->name, name, NAMEMAX);

  memmove(&child->direntry, direntry, sizeof(struct fat_dir_entry));

  child->size = intcopylittle32(direntry->size);

  
  child->startcluster =
    ((uint32_t) intcopylittle16(direntry->cluster_high)) << 16;

  child->startcluster |=
    ((uint32_t) intcopylittle16(direntry->cluster_low));

  memmove(&attr, &direntry->attr, sizeof(uint8_t));

  if (attr & FAT_ATTR_read_only) {
    child->attr = ATTR_rd;
  } else {
    child->attr = ATTR_rd | ATTR_wr;
  }

  if (attr & FAT_ATTR_directory) {
    child->attr |= ATTR_dir;

    child->dirbuf = builddirbuf(fat, child->startcluster,
				&child->dirbuflen);

    if (child->dirbuf == nil) {
      *err = ENOMEM;
      printf("fat mount failed to read dir\n");
      free(child);
      return nil;
    }
  }

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
  uint32_t cluster, rlen, tlen;

  if (offset >= file->size) {
    *err = EOF;
    return 0;
  } else if (offset + len >= file->size) {
    len = file->size - offset;
  }

  tlen = 0;
  for (cluster = file->startcluster;
       cluster != 0 && tlen < len;
       cluster = nextcluster(fat, cluster)) {
       
    if (offset > fat->spc * fat->bps) {
      offset -= fat->spc * fat->bps;
      continue;
    }

    if (!readsectors(fat, clustertosector(fat, cluster))) {
      *err = ERR;
    }

    if (offset + (len - tlen) >= fat->spc * fat->bps) {
      rlen = fat->spc * fat->bps - offset;
    } else {
      rlen = len - tlen;
    }

    memmove(buf, fat->buf + offset, rlen);
    offset = 0;
    
    tlen += rlen;
    buf += rlen;
  }

  *err = OK;
  return tlen;
}

int
fatreaddir(struct fat *fat, struct fat_file *file,
	   uint8_t *buf, uint32_t offset, uint32_t len,
	   int *err)
{
  if (offset >= file->dirbuflen) {
    *err = EOF;
    return 0;
  } else if (offset + len > file->dirbuflen) {
    len = file->dirbuflen - offset;
  }

  *err = OK;
  memmove(buf, file->dirbuf + offset, len);
  return len;
}

uint8_t *
builddirbufsectors(struct fat *fat, uint32_t sector,
		   uint8_t *buf, size_t *len, size_t *mlen)
{
  struct fat_dir_entry *entry;
  char name[NAMEMAX];
  uint8_t *nbuf, l;

  if (!readsectors(fat, sector)) {
    return nil;
  }

  entry = (struct fat_dir_entry *) fat->buf;
    
  while ((uint8_t *) entry < fat->buf + fat->spc * fat->bps) {
    entry = copyfileentryname(entry, name);
    if (entry == nil) {
      return buf;
    }

    l = strlen(name) + 1;

    if (*len + 1 + l >= *mlen) {
      *mlen = *mlen + 1 + l;
      nbuf = malloc(*mlen);
      if (nbuf == nil) {
	return nil;
      }

      if (buf != nil) {
	memmove(nbuf, buf, *len);
	free(buf);
      }
	
      buf = nbuf;
    }

    memmove(buf + *len, &l, sizeof(uint8_t));
    *len += 1;

    memmove(buf + *len, name, l);
    *len += l;
  }

  return buf;
} 

uint8_t *
builddirbuf(struct fat *fat, uint32_t cluster, size_t *len)
{
  uint8_t *buf;
  size_t mlen;

  mlen = 1;
  buf = nil;

  *len = 0;
  while (cluster != 0) {
    buf = builddirbufsectors(fat, clustertosector(fat, cluster),
			     buf, len, &mlen);

    if (buf == nil) {
      return nil;
    }
    
    cluster = nextcluster(fat, cluster);
  }

  return buf;
}
