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
  fat->files->attr = ATTR_wr|ATTR_rd|ATTR_dir;
  fat->files->parent = nil;
  fat->files->children = nil;
  fat->files->cnext = nil;

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

  } else {
    fat->type = FAT32;

    fat->spf = intcopylittle32(bs.ext.high.spf);

    /* This should work for now */
    fat->files->size = fat->spc * fat->bps;

    fat->dataarea = fat->reserved + fat->spf * fat->nft;

    fat->files->startcluster =
      intcopylittle32(bs.ext.high.rootcluster);
  }

  fat->nclusters = (fat->nsectors - fat->dataarea) / fat->spc;

  return fat;
}

struct fat_dir_entry *
fatfindfileincluster(struct fat *fat, uint32_t sector, char *name)
{
  struct fat_dir_entry *file;
  char fname[NAMEMAX];

  if (!readsectors(fat, sector, fat->spc)) {
    return nil;
  }
 
  file = (struct fat_dir_entry *) fat->buf;
  while ((uint8_t *) file < fat->buf + fat->spc * fat->bps) {

    file = copyfileentryname(file, fname);
    if (file == nil) {
      return nil;
    } else if (strncmp(fname, name, NAMEMAX)) {
      return file;
    } else {
      file++;
    }
  }

  return nil;
}

struct fat_dir_entry *
fatfindemptydirentryincluster(struct fat *fat, uint32_t sector)
{
  struct fat_dir_entry *file;

  if (!readsectors(fat, sector, fat->spc)) {
    return nil;
  }
 
  file = (struct fat_dir_entry *) fat->buf;
  while ((uint8_t *) file < fat->buf + fat->spc * fat->bps) {
    if (file->name[0] == 0) {
      return file;
    }

    file++;
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

  child = fatfilefromentry(fat, direntry, name);
  if (child == nil) {
    *err = ENOMEM;
    return nil;
  }

  child->parent = parent;

  child->cnext = parent->children;
  parent->children = child;

  *err = OK;
  return child;
}

struct fat_file *
fatfilefromentry(struct fat *fat, struct fat_dir_entry *entry,
		 char *name)
{
  struct fat_file *f;
  uint8_t attr;
  
  f = malloc(sizeof(struct fat_file));
  if (f == nil) {
    return nil;
  }

  f->fid = fat->nfid++;

  strlcpy(f->name, name, NAMEMAX);

  f->dirbuf = nil;
  f->parent = nil;
  f->children = nil;
  f->cnext = nil;

  memmove(&f->direntry, entry, sizeof(struct fat_dir_entry));

  f->size = intcopylittle32(entry->size);
  
  f->startcluster =
    ((uint32_t) intcopylittle16(entry->cluster_high)) << 16;

  f->startcluster |=
    ((uint32_t) intcopylittle16(entry->cluster_low));

  memmove(&attr, &entry->attr, sizeof(uint8_t));

  if (attr & FAT_ATTR_read_only) {
    f->attr = ATTR_rd;
  } else {
    f->attr = ATTR_rd | ATTR_wr;
  }

  if (attr & FAT_ATTR_directory) {
    f->attr |= ATTR_dir;
  }

  return f;
}

int
fatfileread(struct fat *fat, struct fat_file *file,
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

    if (!readsectors(fat, clustertosector(fat, cluster), fat->spc)) {
      *err = ERR;
    }

    if (offset + len >= fat->spc * fat->bps) {
      rlen = fat->spc * fat->bps - offset;
    } else {
      rlen = len;
    }

    memmove(buf, fat->buf + offset, rlen);
    offset = 0;

    len -= rlen;
    tlen += rlen;
    buf += rlen;
  }

  *err = OK;
  return tlen;
}

int
fatfilewrite(struct fat *fat, struct fat_file *file,
	    uint8_t *buf, uint32_t offset, uint32_t len,
	    int *err)
{
  uint32_t cluster, n, rlen, tlen, sector;

  tlen = 0;
  cluster = file->startcluster;

  while (cluster != 0) {
    if (offset > fat->spc * fat->bps) {
      offset -= fat->spc * fat->bps;
      cluster = nextcluster(fat, cluster);
      continue;
    }

    sector = clustertosector(fat, cluster);

    if (offset > 0 || len - tlen < fat->spc * fat->bps) {
      /* Read sector if we are using it */
      if (!readsectors(fat, sector, fat->spc)) {
	*err = ERR;
      }
    }

    if (offset + (len - tlen) >= fat->spc * fat->bps) {
      rlen = fat->spc * fat->bps - offset;
    } else {
      rlen = len - tlen;
    }

    memmove(fat->buf + offset, buf, rlen);

    if (!writesectors(fat, sector, fat->spc)) {
      *err = ERR;
    }

    offset = 0;
    tlen += rlen;
    buf += rlen;

    if (tlen == len) {
      break;
    } else {
      n = nextcluster(fat, cluster);

      if (n != 0) {
	cluster = n;
	continue;
      }

      /* Need to find another cluster to use */

      n = findfreecluster(fat);

      writetableinfo(fat, cluster, n);

      cluster = n;
    }
  }

  if (tlen > 0) {
    *err = OK;
    return tlen;
  } else {
    *err = EOF;
    return 0;
  }
}

int
fatdirread(struct fat *fat, struct fat_file *file,
	   uint8_t *buf, uint32_t offset, uint32_t len,
	   int *err)
{
  if (file->dirbuf == nil) {
    if (!rebuilddirbuf(fat, file)) {
      *err = ENOMEM;
      return 0;
    }
  }

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

int
fatfileremove(struct fat *fat, struct fat_file *file)
{
  struct fat_dir_entry *direntry;
  struct fat_file *parent;
  uint32_t cluster, sector, n;

  if (file->attr & ATTR_dir) {
    if (file->dirbuf == nil) {
      if (!rebuilddirbuf(fat, file)) {
	return ENOMEM;
      }
    }

    if (file->dirbuflen > 0) {
      return ENOTEMPTY;
    }
  }
  
  parent = file->parent;
  if (parent == nil) {
    return ERR;
  }
  
  direntry = nil;

  if (parent->startcluster == 0) {
    sector = fat->rootdir;
    direntry = fatfindfileincluster(fat, sector, file->name);
  } else {
    cluster = parent->startcluster;
    while (cluster != 0) {
      sector = clustertosector(fat, cluster);
      direntry = fatfindfileincluster(fat, sector, file->name);
      if (direntry != nil) {
	break;
      } else {
	cluster = nextcluster(fat, cluster);
      }
    }
  }

  if (direntry == nil) {
    printf("remove didn't find dir entry\n");
    return ENOFILE;
  }

  memmove(direntry, direntry + 1,
	  fat->spc * fat->bps - ((uint8_t *) direntry - fat->buf));

  if (!writesectors(fat, sector, fat->spc)) {
    printf("fat mount failed to write modified dir.\n");
    return ERR;
  }

  cluster = file->startcluster;
  while (cluster != 0) {
    n = nextcluster(fat, cluster);
    writetableinfo(fat, cluster, 0);
    cluster = n;
  }

  if (parent->dirbuf != nil) {
    free(parent->dirbuf);
    parent->dirbuf = nil;
  }

  return OK;
}

struct fat_file *
fatfilecreate(struct fat *fat, struct fat_file *parent,
	      char *name, uint32_t attr)
{
  struct fat_dir_entry *direntry;
  struct fat_file *new;
  uint32_t cluster, sector, newcluster;
  uint8_t fattr;
  int i, j;

  newcluster = findfreecluster(fat);
  if (newcluster == 0) {
    return nil;
  }

  if (parent->startcluster == 0) {
    sector = fat->rootdir;
    direntry = fatfindemptydirentryincluster(fat, sector);
  } else {
    cluster = parent->startcluster;
    while (cluster != 0) {
      sector = clustertosector(fat, cluster);
      direntry = fatfindemptydirentryincluster(fat, sector);
      if (direntry != nil) {
	break;
      } else {
	cluster = nextcluster(fat, cluster);
      }
    }
  }

  if (direntry == nil) {
    printf("create out of space, need to exspand dir. not yet implimented\n");
    writetableinfo(fat, newcluster, 0);
    return nil;
  }

  i = 0;

  for (j = 0; j < sizeof(direntry->name); j++) {
    if (name[i] == 0 || name[i] == '.') {
      direntry->name[j] = ' ';
    } else {
      direntry->name[j] = name[i++];
    }
  }

  if (name[i] == '.')
    i++;
  
  for (j = 0; j < sizeof(direntry->ext); j++) {
    if (name[i] == 0) {
      direntry->ext[j] = ' ';
    } else {
      direntry->ext[j] = name[i++];
    }
  }

  if (attr & ATTR_wr) {
    fattr = 0;
  } else {
    fattr = FAT_ATTR_read_only;
  }

  if (attr & ATTR_dir) {
    fattr |= FAT_ATTR_directory;
  }

  memmove(&direntry->attr, &fattr, sizeof(fattr));

  intwritelittle16(direntry->cluster_low,
		   (uint16_t) (newcluster & 0xffff));

  intwritelittle16(direntry->cluster_high,
		   (uint16_t) (newcluster >> 16));

  
  if (!writesectors(fat, sector, fat->spc)) {
    printf("fat mount failed to write updated dir.\n");
    writetableinfo(fat, newcluster, 0);
    return nil;
  }

  memset(fat->buf, 0, fat->spc * fat->bps);

  if (!writesectors(fat, newcluster, fat->spc)) {
    printf("fat mount failed to zero new cluster.\n");
    return nil;
  }

  /* Ignore time for now */

  new = fatfilefromentry(fat, direntry, name);
  if (new == nil) {
    printf("fat mount failed to create fat_file\n");
    return nil;
  }
  
  new->parent = parent;

  new->cnext = parent->children;
  parent->children = new;

  if (parent->dirbuf != nil) {
    free(parent->dirbuf);
    parent->dirbuf = nil;
  }

  return new;
}

