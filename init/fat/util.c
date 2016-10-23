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
fatfileclunk(struct fat *fat, struct fat_file *f)
{
  memset(f, 0, sizeof(struct fat_file));
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

void
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

uint32_t
fatfilefromentry(struct fat *fat, struct fat_dir_entry *entry,
		 char *name, struct fat_file *parent)
{
  struct fat_file *f;
  uint8_t attr;
  int i;

  /* Skip root fid and search for empty slot */
  for (i = 1; i < FIDSMAX; i++) {
    if (fat->files[i].name[0] == 0) {
      f = &fat->files[i];
      break;
    }
  }

  if (i == FIDSMAX) {
    printf("fat mount fid table full!\n");
    return nil;
  }

  strlcpy(f->name, name, NAMEMAX);

  f->parent = parent;

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
    f->size = fat->spc * fat->bps;
  }

  return i;
}

