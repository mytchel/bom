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

#define clustertosector(fat, cluster) \
  (fat->dataarea + ((cluster - 2) * fat->spc))

static bool
readsectors(struct fat *fat, uint32_t sector, size_t n);

static bool
writesectors(struct fat *fat, uint32_t sector, size_t n);

static uint32_t
nextcluster(struct fat *fat, uint32_t prev);

static uint32_t
findfreecluster(struct fat *fat);

static uint32_t
tableinfo(struct fat *fat, uint32_t prev);

static void
writetableinfo(struct fat *fat, uint32_t cluster, uint32_t v);

static struct fat_dir_entry *
fatfindfileincluster(struct fat *fat, uint32_t sector, char *name);

static struct fat_dir_entry *
fatfindemptydirentryincluster(struct fat *fat, uint32_t sector);

static struct fat_dir_entry *
copyfileentryname(struct fat_dir_entry *start, char *name);

static uint8_t *
builddirbufsectors(struct fat *fat, uint32_t sector,
		   uint8_t *buf, size_t *len, size_t *mlen);

static bool
builddirbuf(struct fat *fat, struct fat_file *f);

static struct fat_file *
fatfilefromentry(struct fat *fat, struct fat_dir_entry *entry,
		 char *name);


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

struct fat_file *
fatfindfidh(struct fat_file *f, uint32_t fid)
{
  struct fat_file *c;

  if (f == nil) {
    return nil;
  } else if (f->fid == fid) {
    return f;
  } else {
    c = fatfindfidh(f->children, fid);
    if (c == nil) {
      c = fatfindfidh(f->cnext, fid);
    }

    return c;
  }
}

struct fat_file *
fatfindfid(struct fat *fat, uint32_t fid)
{
  return fatfindfidh(fat->files, fid);
}

void
fatfileclunk(struct fat *fat, struct fat_file *f)
{
  struct fat_file *c;

  if (f->parent->children == f) {
    f->parent->children = f->cnext;
  } else {
    for (c = f->parent->children; c->cnext != f; c = c->cnext)
      ;

    c->cnext = f->cnext;
  }

  for (c = f->children; c != nil; c = c->cnext)
    fatfileclunk(fat, c);
  
  if (f->dirbuf != nil) {
    free(f->dirbuf);
  }

  free(f);
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

static uint32_t
findfreecluster(struct fat *fat)
{
  uint32_t sec, off, v;

  if (seek(fat->fd, fat->reserved * fat->bps, SEEK_SET) < 0) {
    printf("fat mount failed to seek.\n");
    return 0;
  }

  for (sec = 0; sec < fat->spf; sec++) {
    if (read(fat->fd, fat->buf, fat->bps) != fat->bps) {
      printf("fat mount failed to read table!\n");
      return 0;
    }

    off = 0;
    while (off < fat->bps) {
      switch (fat->type) {
      case FAT16:
	v = intcopylittle16(fat->buf + off);
	if (v == 0) {
	  v = FAT16_END;
	  memmove(fat->buf + off, &v, sizeof(uint16_t));
	  goto found;
	} else {
	  off += sizeof(uint16_t);
	}
	break;
      case FAT32:
	v = intcopylittle32(fat->buf + off);
	if (v == 0) {
	  v = FAT32_END;
	  memmove(fat->buf + off, &v, sizeof(uint32_t));
	  goto found;
	} else {
	  off += sizeof(uint32_t);
	}
	break;
      }
    }
  }

  return 0;

  found:

  if (seek(fat->fd, -fat->bps, SEEK_CUR) < 0) {
    printf("fat mount failed to seek back sector.\n");
    return 0;
  }
  
  if (write(fat->fd, fat->buf, fat->bps) != fat->bps) {
    printf("fat mount failed to write modified fat table.\n");
    return 0;
  }

  switch (fat->type) {
  case FAT16:
    return (sec * fat->bps + off) / sizeof(uint16_t);
  case FAT32:
    return (sec * fat->bps + off) / sizeof(uint32_t);
  default:
    return 0;
  }
}

static void
writetableinfo(struct fat *fat, uint32_t cluster, uint32_t v)
{
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

  if (!readsectors(fat, fat->reserved + sec, 1)) {
    printf("fat mount failed to seek.\n");
    return;
  }

  off = off % fat->bps;

  switch (fat->type) {
  case FAT16:
    intwritelittle16(fat->buf + off, (uint16_t) v);
    break;
  case FAT32:
    intwritelittle32(fat->buf + off, v);
    break;
  }

  if (!writesectors(fat, fat->reserved + sec, 1)) {
    printf("fat mount failed to write altered table!\n");
    return;
  }
}

uint32_t
tableinfo(struct fat *fat, uint32_t cluster)
{
  uint32_t v = 0;
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

  if (!readsectors(fat, fat->reserved + sec, 1)) {
    return 0;
  }

  off = off % fat->bps;

  switch (fat->type) {
  case FAT16:
    v = intcopylittle16(fat->buf + off);
    break;
  case FAT32:
    v = intcopylittle32(fat->buf + off);
    break;
  }

  return v;
}

bool
readsectors(struct fat *fat, uint32_t sector, size_t n)
{
  int i;

  if (fat->bufsector == sector) {
    return true;
  }
  
  fat->bufsector = sector;
  
  if (seek(fat->fd, sector * fat->bps, SEEK_SET) < 0) {
    printf("fat mount failed to seek to sector %i\n", sector);
    return false;
  }

  for (i = 0; i < n; i++) {
    if (read(fat->fd, fat->buf + i * fat->bps, fat->bps) < 0) {
      printf("fat mount failed to read sector %i\n", sector + i);
      return false;
    }
  }

  return true;
}

bool
writesectors(struct fat *fat, uint32_t sector, size_t n)
{
  int i;
  
  if (seek(fat->fd, sector * fat->bps, SEEK_SET) < 0) {
    printf("fat mount failed to seek to sector %i\n", sector);
    return false;
  }

  for (i = 0; i < n; i++) {
    if (write(fat->fd, fat->buf + i * fat->bps, fat->bps) < 0) {
      printf("fat mount failed to write sector %i\n", sector + i);
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
    return file;
  }
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

uint8_t *
builddirbufsectors(struct fat *fat, uint32_t sector,
		   uint8_t *buf, size_t *len, size_t *mlen)
{
  struct fat_dir_entry *entry;
  char name[NAMEMAX];
  uint8_t *nbuf, l;

  if (!readsectors(fat, sector, fat->spc)) {
    return nil;
  }

  entry = (struct fat_dir_entry *) fat->buf;
    
  while ((uint8_t *) entry < fat->buf + fat->spc * fat->bps) {
    entry = copyfileentryname(entry, name);
    if (entry == nil) {
      return buf;
    } else {
      entry++;
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

bool
builddirbuf(struct fat *fat, struct fat_file *f)
{
  uint32_t cluster;
  size_t mlen;

  mlen = 1;
  f->dirbuflen = 0;

  if (f->startcluster == 0) {
    f->dirbuf =
      builddirbufsectors(fat, fat->rootdir,
			 f->dirbuf, &f->dirbuflen, &mlen);

    if (f->dirbuf == nil) {
      return false;
    }
  } else {
    cluster = f->startcluster;
    
    while (cluster != 0) {
      f->dirbuf =
	builddirbufsectors(fat, clustertosector(fat, cluster),
			   f->dirbuf, &f->dirbuflen, &mlen);

      if (f->dirbuf == nil) {
	return false;
      }
      
      cluster = nextcluster(fat, cluster);
    }
  }

  return true;
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
    if (!builddirbuf(fat, file)) {
      *err = ENOMEM;
      printf("fat mount failed to read dir\n");
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
      if (!builddirbuf(fat, file)) {
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

  printf("modify buffer\n");

  memmove(direntry, direntry + 1,
	  fat->spc * fat->bps - ((uint8_t *) direntry - fat->buf));

  printf("write buffer\n");
  if (!writesectors(fat, sector, fat->spc)) {
    printf("fat mount failed to write modified dir.\n");
    return ERR;
  }

  printf("free clusters\n");
  
  cluster = file->startcluster;
  while (cluster != 0) {
    n = nextcluster(fat, cluster);
    printf("next at %i\n", n);
    writetableinfo(fat, cluster, 0);
    cluster = n;
  }

  printf("removed\n");

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

