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

  fat->fd = fd;

  memset(fat->files, 0, sizeof(fat->files));
  
  fat->files[0].attr = ATTR_wr|ATTR_rd|ATTR_dir;

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

  fatinitbufs(fat);

  return fat;
}

static struct fat_dir_entry *
fatfindfileincluster(struct fat *fat, uint8_t *buf, char *name)
{
  struct fat_dir_entry *file;
  char fname[NAMEMAX];

  file = (struct fat_dir_entry *) buf;
  while ((uint8_t *) file < buf + fat->spc * fat->bps) {

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

uint32_t
fatfilefind(struct fat *fat, struct fat_file *parent,
	    char *name, int *err)
{
  struct fat_dir_entry *direntry;
  struct buf *buf;
  uint32_t cluster;
  uint32_t child;
  int i;

  for (i = 1; i < FIDSMAX; i++) {
    if (fat->files[i].parent != parent) continue;
    if (fat->files[i].refs != 0) continue;
    if (strncmp(fat->files[i].name, name, NAMEMAX)) {
      /* Slot was clunked but still in array */
      *err = OK;
      return i;
    }
  }

  direntry = nil;

  if (parent->startcluster == 0) {
    buf = readsectors(fat, fat->rootdir, fat->spc);
    if (buf == nil) {
      return nil;
    }
 
    direntry = fatfindfileincluster(fat, buf->addr, name);
  } else {
    cluster = parent->startcluster;
    while (cluster != 0) {
      buf = readsectors(fat, clustertosector(fat, cluster), fat->spc);
      if (buf == nil) {
	return nil;
      }

      direntry =
	fatfindfileincluster(fat, buf->addr, name);

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

  child = fatfilefromentry(fat, buf, direntry, name, parent);
  if (child == 0) {
    *err = ENOMEM;
  } else {
    *err = OK;
  }

  return child;
}

int
fatfileread(struct fat *fat, struct fat_file *file,
	    uint8_t *buf, uint32_t offset, uint32_t len,
	    int *err)
{
  uint32_t cluster, rlen, tlen;
  struct buf *fbuf;
  
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

    fbuf = readsectors(fat, clustertosector(fat, cluster), fat->spc);
    if (fbuf == nil) {
      *err = ERR;
    }

    if (offset + len >= fat->spc * fat->bps) {
      rlen = fat->spc * fat->bps - offset;
    } else {
      rlen = len;
    }

    memmove(buf, fbuf->addr + offset, rlen);
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
  uint32_t cluster, n, rlen, tlen, pos;
  struct buf *fbuf;

  pos = 0;
  tlen = 0;
  *err = OK;
  cluster = file->startcluster;
  while (cluster != 0) {
    if (offset > pos + fat->spc * fat->bps) {
      pos += fat->spc * fat->bps;
      cluster = nextcluster(fat, cluster);
      continue;
    }

    fbuf = readsectors(fat, clustertosector(fat, cluster),
		       fat->spc);
    if (fbuf == nil) {
      *err = ERR;
      break;
    }

    rlen = len - tlen;
    if (pos >= offset) {
      if (rlen > fat->spc * fat->bps) {
	rlen = fat->spc * fat->bps;
      } 

      memmove(fbuf->addr, buf, rlen);
    } else {
      if ((offset - pos) + rlen > fat->spc * fat->bps) {
	rlen = fat->spc * fat->bps - (offset - pos);
      }

      memmove(fbuf->addr + (offset - pos), buf, rlen);
      pos = offset;
    }
    
    if (!writesectors(fat, fbuf, fat->spc)) {
      printf("fat mount failed to write updated sectors\n");
      forcerereadbuf(fat, fbuf);
      *err = ERR;
      break;
    }

    pos += rlen;
    tlen += rlen;
    buf += rlen;

    if (tlen == len) {
      break;
    } else {
      n = nextcluster(fat, cluster);

      if (n == 0) {
	/* Need to find another cluster to use */
	n = findfreecluster(fat);

	if (!writetableinfo(fat, cluster, n)) {
	  printf("failed to update file info\n");
	  *err = ERR;
	  break;
	}
      }

      cluster = n;
    }
  }

  if (pos > file->size) {
    file->size = pos;

    if (!fatupdatedirentry(fat, file)) {
      printf("fat mount failed to update dir entry!\n");
      *err = ERR;
    }
  }

  if (*err == OK) {
    return tlen;
  } else {
    return 0;
  }
}

size_t
readdirsectors(struct fat *fat, uint32_t sector,
	       uint8_t *buf, uint32_t *offset, uint32_t len)
{
  struct fat_dir_entry *entry;
  char name[NAMEMAX];
  struct buf *fbuf;
  size_t tlen;
  uint8_t l;

  fbuf = readsectors(fat, sector, fat->spc);
  if (fbuf == nil) {
    return nil;
  }

  tlen = 0;
  entry = (struct fat_dir_entry *) fbuf->addr;
    
  while ((uint8_t *) entry < fbuf->addr + fat->spc * fat->bps) {
    entry = copyfileentryname(entry, name);
    if (entry == nil) {
      break;
    } else {
      entry++;
    }

    l = strlen(name) + 1;

    if (*offset >= l + sizeof(uint8_t)) {
      *offset -= l + sizeof(uint8_t);
      continue;
    }

    if (*offset == 0) { 
      memmove(buf + tlen, &l, sizeof(uint8_t));
      tlen += sizeof(uint8_t);
    } else {
      *offset -= sizeof(uint8_t);
    }
    
    if (tlen >= len) {
      break;
    } else if (tlen + l > len) {
      l = len - tlen;
    }

    if (*offset == 0) {
      memmove(buf + tlen, name, l);
    } else {
      l -= *offset;
      memmove(buf + tlen, name + *offset, l);
      *offset = 0;
    }
    
    tlen += l;

    if (tlen >= len) {
      break;
    }
  }

  return tlen;
} 

int
fatdirread(struct fat *fat, struct fat_file *file,
	   uint8_t *buf, uint32_t offset, uint32_t len,
	   int *err)
{
  uint32_t cluster;
  size_t tlen, t;

  tlen = 0;
  
  if (file->startcluster == 0) {
    tlen = readdirsectors(fat, fat->rootdir, buf, &offset, len);
  } else {
    cluster = file->startcluster;
    
    while (cluster != 0) {
      t = readdirsectors(fat, clustertosector(fat, cluster),
			 buf + tlen, &offset, len - tlen);

      if (t > 0) {
	tlen += t;
	cluster = nextcluster(fat, cluster);
      } else {
	break;
      }
    }
  }

  if (tlen > 0) {
    *err = OK;
  } else {
    *err = EOF;
  }

  return tlen;
}

int
fatfileremove(struct fat *fat, struct fat_file *file)
{
  struct fat_dir_entry *direntry;
  struct fat_file *parent;
  uint32_t cluster, n;
  struct buf *buf;

  if (file->attr & ATTR_dir) {
    /* Need to check if the dir is empty somehow */
  }
  
  parent = file->parent;
  if (parent == nil) {
    return ERR;
  }
  
  direntry = nil;

  if (parent->startcluster == 0) {
    buf = readsectors(fat, fat->rootdir, fat->spc);
    if (buf == nil) {
      return ERR;
    }

    direntry = fatfindfileincluster(fat, buf->addr, file->name);
  } else {
    cluster = parent->startcluster;
    while (cluster != 0) {
      buf = readsectors(fat, clustertosector(fat, cluster), fat->spc);
      if (buf == nil) {
	return ERR;
      }

      direntry = fatfindfileincluster(fat, buf->addr, file->name);
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

  cluster = file->startcluster;
  while (cluster != 0) {
    n = nextcluster(fat, cluster);
    writetableinfo(fat, cluster, 0);
    cluster = n;
  }

  memmove(direntry, direntry + 1,
	  fat->spc * fat->bps - ((uint8_t *) direntry - buf->addr));

  if (!writesectors(fat, buf, fat->spc)) {
    printf("fat mount failed to write modified dir.\n");
    forcerereadbuf(fat, buf);
    return ERR;
  }

  file->refs = 0;

  return OK;
}

static struct fat_dir_entry *
fatfindemptydirentryincluster(struct fat *fat, uint8_t *buf)
{
  struct fat_dir_entry *file;

  file = (struct fat_dir_entry *) buf;
  while ((uint8_t *) file < buf + fat->spc * fat->bps) {
    if (file->name[0] == 0) {
      return file;
    }

    file++;
  }

  return nil;
}

uint32_t
fatfilecreate(struct fat *fat, struct fat_file *parent,
	      char *name, uint32_t attr)
{
  struct fat_dir_entry *direntry;
  uint32_t cluster, newcluster;
  struct fat_file *f;
  struct buf *buf;
  int i;

  newcluster = findfreecluster(fat);
  if (newcluster == 0) {
    return 0;
  }

  if (parent->startcluster == 0) {
    buf = readsectors(fat, fat->rootdir, fat->spc);
    if (buf == nil) {
      return 0;
    }
    
    direntry = fatfindemptydirentryincluster(fat, buf->addr);
  } else {
    cluster = parent->startcluster;
    while (cluster != 0) {
      buf = readsectors(fat, clustertosector(fat, cluster), fat->spc);
      if (buf == nil) {
	return 0;
      }

      direntry = fatfindemptydirentryincluster(fat, buf->addr);
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
    return 0;
  }

  i = fatfindfreefid(fat);
  if (i == 0) {
    printf("fat mount out of fid spaces!\n");
    return 0;
  }
  
  f = &fat->files[i];

  f->refs = 1;
  f->parent = parent;

  strlcpy(f->name, name, NAMEMAX);

  f->direntrysector = buf->sector;
  f->direntryoffset = (uint32_t) direntry - (uint32_t) buf->addr;
  
  f->startcluster = newcluster;

  f->attr = attr;
  if (attr & FAT_ATTR_directory) {
    f->size = fat->spc * fat->bps;
  } else {
    f->size = 0;
  }

  if (!fatupdatedirentry(fat, f)) {;
    printf("fat mount failed to update dir entry\n");
    forcerereadbuf(fat, buf);
    return 0;
  } else {
    return i;
  }
}
