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
nextcluster(struct fat *fat, uint32_t prev);

static struct fat_dir_entry *
fatfindfileincluster(struct fat *fat, uint32_t sector,
		     uint8_t *buf, /* Of len fat->spc * fat->bps */
		     char *name);

static struct fat_dir_entry *
copyfileentryname(struct fat_dir_entry *start, char *name);

struct fat *
fatinit(int fd)
{
  uint32_t reservedsectors;
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

  fat->nfid = ROOTFID;

  fat->files->fid = fat->nfid++;
  fat->files->opened = 0;
  fat->files->fatattr = 0;
  fat->files->attr = ATTR_wr|ATTR_rd|ATTR_dir;
  fat->files->size = 0;
  fat->files->next = nil;

  if (read(fd, &fat->bs, sizeof(fat->bs)) != sizeof(fat->bs)) {
    printf("fat mount failed to read boot sector.\n");
    return nil;
  }

  fat->bps = intcopylittle16(fat->bs.bps);
  fat->spc = fat->bs.spc;

  fat->nsectors = intcopylittle16(fat->bs.sc16);
  if (fat->nsectors == 0) {
    fat->nsectors = intcopylittle32(fat->bs.sc32);
  }

  fat->nft = fat->bs.nft;

  reservedsectors = intcopylittle16(fat->bs.res);

  fat->rde = intcopylittle16(fat->bs.rde);
  if (fat->rde > 0) {
    fat->files->startcluster = 0;

    fat->spf = intcopylittle16(fat->bs.spf);

    fat->rootdir = reservedsectors + fat->spf * fat->nft;
    fat->dataarea = fat->rootdir + (((fat->rde * 32) + (fat->bps - 1))
				  / fat->bps);
  } else {
    fat->spf = intcopylittle32(fat->bs.ext.high.spf);

    fat->files->startcluster =
      intcopylittle32(fat->bs.ext.high.rootcluster);

    fat->dataarea = reservedsectors + fat->spf * fat->nft;
  }

  fat->nclusters = (fat->nsectors - fat->dataarea) / fat->spc;

  if (fat->nclusters < 4085) {
    fat->type = FAT12;
  } else if (fat->nclusters < 65525) {
    fat->type = FAT16;
  } else {
    fat->type = FAT32;
  }

  fat->table = malloc(fat->spf * fat->bps);
  if (fat->table == nil) {
    printf("fat mount failed to alloc table!\n");
    return nil;
  }

  if (seek(fat->fd, reservedsectors * fat->bps, SEEK_SET) < 0) {
    return nil;
  }

  if (read(fd, &fat->table, fat->spf * fat->bps)
      != fat->spf * fat->bps) {
    printf("fat mount failed to read table!\n");
    return nil;
  }
 
  return fat;
}

struct fat_dir_entry *
copyfileentryname(struct fat_dir_entry *file, char *name)
{
  struct fat_lfn *lfn;
  size_t i, j;

  if (file->attr == 0) {
    printf("end of dir\n");
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

  for (j = 0; j < sizeof(file->ext) && file->ext[j] != ' '; j++)
    name[i++] = file->ext[j];

  name[i] = 0;

  return file++;
}

struct fat_dir_entry *
fatfindfileincluster(struct fat *fat, uint32_t sector,
		     uint8_t *buf, /* Of len fat->spc * fat->bps */
		     char *name)
{
  struct fat_dir_entry *file;
  char fname[NAMEMAX];

  if (seek(fat->fd, sector * fat->bps, SEEK_SET) < 0) {
    return nil;
  }

  if (read(fat->fd, buf, fat->spc * fat->bps) < 0) {
    printf("fat mount failed to read dir sector\n");
    return nil;
  }

  printf("start reading dir\n");
  
  file = (struct fat_dir_entry *) buf;
  while ((uint8_t *) file < buf + fat->spc * fat->bps) {
    file = copyfileentryname(file, fname);
    printf("have entry name = '%s'\n", fname);
    if (file == nil) {
      break;
    } else if (strncmp(fname, name, NAMEMAX)) {
      return file;
    }
  }

  printf("finished reading dir\n");

  return nil;
}

struct fat_file *
fatfilefind(struct fat *fat, struct fat_file *parent,
	    char *name, int *err)
{
  struct fat_dir_entry *direntry;
  struct fat_file *child;
  uint32_t cluster;
  uint8_t *buf;

  printf("fatfilefind '%s'\n", name);
  
  buf = malloc(sizeof(fat->spc * fat->bps));
  if (buf == nil) {
    *err = ENOMEM;
    return nil;
  }

  direntry = nil;
  if (parent->startcluster == 0) {
    printf("in fat12/16 root dir\n");
    direntry = fatfindfileincluster(fat, fat->rootdir, buf, name);
  } else {
    printf("in normal dir\n");
    cluster = parent->startcluster;
    while (cluster != 0) {
      direntry = fatfindfileincluster(fat,
				      clusteroffset(fat, cluster),
				      buf, name);
      if (direntry != nil) {
	break;
      } else {
	cluster = nextcluster(fat, cluster);
      }
    }
  }

  if (direntry == nil) {
    *err = ENOFILE;
    free(buf);
    return nil;
  }

  printf("have entry\n");

  child = malloc(sizeof(struct fat_file));
  if (child == nil) {
    *err = ENOMEM;
    free(buf);
    return nil;
  }

  child->fid = fat->nfid++;
  child->opened = 0;
  child->fatattr = direntry->attr;

  strlcpy(child->name, name, NAMEMAX);

  printf("file has fid %i and name '%s'\n", child->fid, child->name);

  child->attr = 0;

  if (child->fatattr & FAT_ATTR_read_only) {
    child->attr |= ATTR_rd;
  } else {
    child->attr |= ATTR_rd | ATTR_wr;
  }

  if (child->fatattr & FAT_ATTR_directory) {
    child->attr = ATTR_dir;
  }

  printf("file has attr %b\n", child->attr);

  child->size = intcopylittle32(direntry->size);
  printf("file has size %i\n", child->size);
  
  child->startcluster =
    ((uint32_t) intcopylittle16(direntry->cluster_high)) << 16;
  child->startcluster |=
    ((uint32_t) intcopylittle16(direntry->cluster_low));

  printf("file initial cluster at 0x%h\n", child->startcluster);

  printf("add to list\n");
  child->next = fat->files;
  fat->files = child;

  free(buf);
  *err = OK;
  return child;
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

int
fatreadfile(struct fat *fat, struct fat_file *file,
	    uint8_t *buf, uint32_t offset, uint32_t len)
{
  return ERR;
}

int
fatreaddir(struct fat *fat, struct fat_file *file,
	   uint8_t *buf, uint32_t offset, uint32_t len)
{
  uint32_t cluster;
  uint8_t *cbuf;

  cbuf = malloc(fat->spc * fat->bps);
  if (cbuf == nil) {
    return ENOMEM;
  }
  
  if (file->startcluster == 0) {
    printf("in fat12/16 root dir\n");

    if (seek(fat->fd, fat->rootdir * fat->bps, SEEK_SET) < 0) {
      return nil;
    }

    if (read(fat->fd, cbuf, fat->spc * fat->bps) < 0) {
      printf("fat mount failed to read dir sector\n");
      return nil;
    }

 } else {
    printf("in normal dir\n");
    cluster = file->startcluster;
    while (cluster != 0) {
      break;
      /*
      if (direntry != nil) {
	break;
      } else {
	cluster = nextcluster(fat, cluster);
      }
      */
    }
  }
  
  return ERR;
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

  printf("find next cluster for 0x%h\n", prev);
  v = fat->table[prev];
  printf("has value 0x%h\n", v);

  switch (fat->type) {
  case FAT12:
    v &= 0xfff;
    if (v >= 0xff8) {
      v = 0;
    }
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
