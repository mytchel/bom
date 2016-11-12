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

void
fatinitbufs(struct fat *fat)
{
  void *addr;
  size_t s;
  int i;

  i = 0;
  
  while (true) {
    s = fat->spc * fat->bps;
    addr = mmap(MEM_ram, nil, &s);

    while (s > fat->spc * fat->bps) {
      fat->bufs[i].addr = addr;
      fat->bufs[i].sector = 0;
      fat->bufs[i].uses = 0;
      fat->bufs[i].n = 0;

      addr = (void *) ((uint8_t *) addr + fat->spc * fat->bps);
      s -= fat->spc * fat->bps;

      if (++i == BUFSMAX) {
	return;
      }
    }
  }
}

bool
forcerereadbuf(struct fat *fat, struct buf *buf)
{
  int i;
  
  if (seek(fat->fd, buf->sector * fat->bps, SEEK_SET) < 0) {
    return false;
  }

  for (i = 0; i < buf->n; i++) {
    if (read(fat->fd, (uint8_t *) buf->addr + i * fat->bps,
	     fat->bps) < 0) {

      printf("fat mount read sector %i failed\n",
	     buf->sector + i);
      
      return false;
    }
  }

  return true;
}

struct buf *
readsectors(struct fat *fat, uint32_t sector, size_t n)
{
  struct buf *buf;
  int i;

  buf = nil;
  for (i = 0; i < BUFSMAX; i++) {
    if (fat->bufs[i].sector == sector) {
      fat->bufs[i].uses++;
      if (fat->bufs[i].n >= n) {
	return &fat->bufs[i];
      } else {
	break;
      }
    } else if (buf == nil) {
      buf = &fat->bufs[i];
    } else if (buf->uses > fat->bufs[i].uses) {
      buf = &fat->bufs[i];
    }

    if (buf->uses == 0) {
      break;
    }
  }

  if (buf->sector != sector) {
    buf->sector = sector;
    buf->uses = 1;
  }

  buf->n = n;

  if (forcerereadbuf(fat, buf)) {
    return buf;
  } else {
    buf->sector = 0;
    buf->n = 0;
    buf->uses = 0;
    return nil;
  }
}

bool
writesectors(struct fat *fat, struct buf *buf, size_t n)
{
  int i;
  
  if (seek(fat->fd, buf->sector * fat->bps, SEEK_SET) < 0) {
    return false;
  }

  for (i = 0; i < n; i++) {
    if (write(fat->fd, buf->addr + i * fat->bps, fat->bps) < 0) {
      return false;
    }
  }

  return true;
}

void
fatfileclunk(struct fat *fat, struct fat_file *f)
{
  if (f->refs == 0) {
    /* File removed now wipe everything */
    memset(f, 0, sizeof(struct fat_file));
  } else {
    f->refs = 0;
  }
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
findfreecluster(struct fat *fat)
{
  uint32_t sec, off, v;
  struct buf *buf;

  for (sec = 0; sec < fat->spf; sec += 4) {
    buf = readsectors(fat, fat->reserved + sec, 1);
    if (buf == nil) {
      printf("fat mount failed to read table!\n");
      return 0;
    }

    off = 0;
    while (off < fat->bps) {
      switch (fat->type) {
      case FAT16:
	v = intcopylittle16(buf->addr + off);
	if (v == 0) {
	  v = FAT16_END;
	  memmove(buf->addr + off, &v, sizeof(uint16_t));
	  goto found;
	} else {
	  off += sizeof(uint16_t);
	}
	break;
      case FAT32:
	v = intcopylittle32(buf->addr + off);
	if (v == 0) {
	  v = FAT32_END;
	  memmove(buf->addr + off, &v, sizeof(uint32_t));
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

  if (!writesectors(fat, buf, 1)) {
    printf("fat mount failed to write modified fat table.\n");
    if (!forcerereadbuf(fat, buf)) {
      printf("fat mount failed to read failed write!\n");
      exit(ERR);
    }
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

bool
writetableinfo(struct fat *fat, uint32_t cluster, uint32_t v)
{
  uint32_t sec, off = 0;
  struct buf *buf;

  switch (fat->type) {
  case FAT16:
    off = cluster * sizeof(uint16_t);
    break;
  case FAT32:
    off = cluster * sizeof(uint32_t);
    break;
  }

  sec = off / fat->bps;

  buf = readsectors(fat, fat->reserved + sec, 1);
  if (buf == nil) {
    printf("fat mount failed to read table info.\n");
    return false;
  }

  off = off % fat->bps;

  switch (fat->type) {
  case FAT16:
    intwritelittle16(buf->addr + off, (uint16_t) v);
    break;
  case FAT32:
    intwritelittle32(buf->addr + off, v);
    break;
  }

  if (!writesectors(fat, buf, 1)) {
    printf("fat mount failed to write altered table!\n");
    readsectors(fat, buf->sector, buf->n);
    return false;
  } else {
    return true;
  }
}

uint32_t
tableinfo(struct fat *fat, uint32_t cluster)
{
  struct buf *buf;
  uint32_t sec, off = 0;
  uint32_t v = 0;

  switch (fat->type) {
  case FAT16:
    off = cluster * sizeof(uint16_t);
    break;
  case FAT32:
    off = cluster * sizeof(uint32_t);
    break;
  }

  sec = off / fat->bps;

  buf = readsectors(fat, fat->reserved + sec, 1);
  if (buf == nil) {
    return 0;
  }

  off = off % fat->bps;

  switch (fat->type) {
  case FAT16:
    v = intcopylittle16(buf->addr + off);
    break;
  case FAT32:
    v = intcopylittle32(buf->addr + off);
    break;
  }

  return v;
}

struct fat_dir_entry *
copyfileentryname(struct fat_dir_entry *file, char *name)
{
  struct fat_lfn *lfn;
  size_t i, j;

  if (file->name[0] == 0) {
    return nil;
  } else if (file->name[0] == 0xe5) {
    return copyfileentryname(file + 1, name);
  }
 
  while ((file->attr & FAT_ATTR_lfn) == FAT_ATTR_lfn) {
    printf("This is a long file name entry, not sure what to do\n");

    lfn = (struct fat_lfn *) file++;
    
    printf("order = %i\n", lfn->order);
  } 

  i = 0;

  for (j = 0; j < sizeof(file->name) && file->name[j] != ' '; j++) {
    if (file->name[j] >= 'A' && file->name[j] <= 'Z') {
      /* Convert upper to lower */
      name[i++] = file->name[j] + 32;
    } else {
      name[i++] = file->name[j];
    }
  }

  if (file->ext[0] != ' ') {
    name[i++] = '.';
  
    for (j = 0; j < sizeof(file->ext) && file->ext[j] != ' '; j++) {
      if (file->ext[j] >= 'A' && file->ext[j] <= 'Z') {
	/* Convert upper to lower */
	name[i++] = file->ext[j] + 32;
      } else {
	name[i++] = file->ext[j];
      }
    }
  }
  
  name[i] = 0;

  if (strcmp(name, ".")) {
    return copyfileentryname(file + 1, name);
  } else if (strcmp(name, "..")) {
    return copyfileentryname(file + 1, name);
  } else {
    return file;
  }
}

uint32_t
fatfindfreefid(struct fat *fat)
{
  int i, b;

  /* Skip root fid and search for empty slot */
  b = 0;
  for (i = 1; i < FIDSMAX; i++) {
    if (fat->files[i].name[0] == 0) {
      /* Slot completely unused */
      b = i;
      break;
    } else if (b == 0 && fat->files[i].refs == 0) {
      /* Slot clunked but could be reused */
      b = i;
    }
  }

  if (b == 0) {
    printf("fat mount fid table full!\n");
    return 0;
  } else {
    memset(&fat->files[b], 0, sizeof(struct fat_file));
  }

  return b;
}

uint32_t
fatfilefromentry(struct fat *fat, struct buf *buf,
		 struct fat_dir_entry *entry,
		 char *name, struct fat_file *parent)
{
  struct fat_file *f;
  int i;

  i = fatfindfreefid(fat);
  if (i == 0) {
    return 0;
  }
  
  f = &fat->files[i];
  
  strlcpy(f->name, name, NAMEMAX);

  f->refs = 1;
  f->parent = parent;

  f->size = intcopylittle32(entry->size);
  
  f->startcluster =
    ((uint32_t) intcopylittle16(entry->cluster_high)) << 16;

  f->startcluster |=
    ((uint32_t) intcopylittle16(entry->cluster_low));

  f->direntrysector = buf->sector;
  f->direntryoffset = (uint32_t) entry - (uint32_t) buf->addr;

  if (entry->attr & FAT_ATTR_read_only) {
    f->attr = ATTR_rd;
  } else {
    f->attr = ATTR_rd | ATTR_wr;
  }

  if (entry->attr & FAT_ATTR_directory) {
    f->attr |= ATTR_dir;
    f->dsize = fat->spc * fat->bps;
    f->size = fatdirsize(fat, f);
  } else {
    f->dsize = fatfilesize(fat, f);
  }

  return i;
}

bool
fatupdatedirentry(struct fat *fat, struct fat_file *file)
{
  struct fat_dir_entry *direntry;
  struct buf *buf;
  int i, j;

  buf = readsectors(fat, file->direntrysector,
		    fat->spc);
  if (buf == nil) {
    printf("fat mount failed to read dir entry cluster!\n");
    return false;
  }

  direntry = (struct fat_dir_entry *)
    (buf->addr + file->direntryoffset);

  intwritelittle16(direntry->cluster_high,
		   file->startcluster >> 16);
  intwritelittle16(direntry->cluster_low,
		   file->startcluster & 0xffff);

  direntry->attr = 0;
  if (!(file->attr & ATTR_wr)) {
    direntry->attr |= FAT_ATTR_read_only;
  }

  if (file->attr & ATTR_dir) {
    direntry->attr |= FAT_ATTR_directory;
    memset(direntry->size, 0, sizeof(direntry->size));
  } else {
    intwritelittle32(direntry->size, file->size);
  }

  i = 0;

  for (j = 0; j < sizeof(direntry->name); j++) {
    if (file->name[i] != 0 && file->name[i] != '.') {
      if (file->name[i] >= 'a' && file->name[i] <= 'z') {
	/* Convert lower to upper */
	direntry->name[j] = file->name[i++] - 32;
      } else {
	direntry->name[j] = file->name[i++];
      }

    } else {
      direntry->name[j] = ' ';
    }
  }

  if (file->name[i] == '.') {
    i++;
  }
    
  for (j = 0; j < sizeof(direntry->ext); j++) {
    if (file->name[i] != 0) {
      if (file->name[i] >= 'a' && file->name[i] <= 'z') {
	/* Convert lower to upper */
	direntry->ext[j] = file->name[i++] - 32;
      } else {
	direntry->ext[j] = file->name[i++];
      }
    } else {
      direntry->ext[j] = ' ';
    }
  }

  return writesectors(fat, buf, fat->spc);
}

static uint32_t
nfilesinsector(struct fat *fat, uint32_t sector)
{
  struct fat_dir_entry *entry;
  struct buf *buf;
  char name[NAMEMAX];
  uint32_t s;

  buf = readsectors(fat, sector, fat->spc);
  if (buf == nil) {
    return 0;
  }

  s = 0;
  entry = (struct fat_dir_entry *) buf->addr;
    
  while ((uint8_t *) entry < buf->addr + fat->spc * fat->bps) {
    entry = copyfileentryname(entry, name);
    if (entry == nil) {
      break;
    } else {
      s++;
      entry++;
    }
  }

  return s;
}

uint32_t
fatdirsize(struct fat *fat, struct fat_file *file)
{
  uint32_t cluster;
  size_t s;

  s = 0;
  if (file->startcluster == 0) {
    s = nfilesinsector(fat, fat->rootdir);
  } else {
    cluster = file->startcluster;

    while (cluster != 0) {
      s += nfilesinsector(fat, clustertosector(fat, cluster));
      cluster = nextcluster(fat, cluster);
    }
  }

  return s;
}

uint32_t
fatfilesize(struct fat *fat, struct fat_file *file)
{
  uint32_t cluster, s;

  s = 0;
  if (file->startcluster == 0) {
    s = fat->bps * fat->spc;
  } else {
    cluster = file->startcluster;

    while (cluster != 0) {
      s += fat->bps * fat->spc;
      cluster = nextcluster(fat, cluster);
    }
  }

  return s;
}

