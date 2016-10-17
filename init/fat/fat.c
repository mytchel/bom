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

struct fat fat;

int
fatinit(int fd)
{
  struct fat_dir_entry *file;
  struct fat_lfn *lfn;
  uint32_t reservedsectors;
  uint32_t s;
  uint8_t *buf;

  if (read(fd, &fat.bs, sizeof(fat.bs)) != sizeof(fat.bs)) {
    printf("Failed to read boot sector.\n");
    return ERR;
  }

  fat.bps = intcopylittle16(fat.bs.bps);
  fat.spc = fat.bs.spc;

  fat.nsectors = intcopylittle16(fat.bs.sc16);
  if (fat.nsectors == 0) {
    fat.nsectors = intcopylittle32(fat.bs.sc32);
  }

  fat.nft = fat.bs.nft;

  reservedsectors = intcopylittle16(fat.bs.res);

  fat.rde = intcopylittle16(fat.bs.rde);
  if (fat.rde > 0) {
    fat.spf = intcopylittle16(fat.bs.spf);

    fat.rootdir = reservedsectors + fat.spf * 2;
    fat.dataarea = fat.rootdir + (((fat.rde * 32) + (fat.bps - 1))
				  / fat.bps);
  } else {
    fat.spf = intcopylittle32(fat.bs.ext.high.spf);

    fat.rootdir = intcopylittle32(fat.bs.ext.high.rootcluster)
      * fat.spc;
    fat.dataarea = reservedsectors + fat.spf * 2;
  }

  fat.nclusters = (fat.nsectors - fat.dataarea) / fat.spc;

  if (fat.nclusters < 4085) {
    fat.type = FAT12;
  } else if (fat.nclusters < 65525) {
    fat.type = FAT16;
  } else {
    fat.type = FAT32;
  }

  if (seek(fd, fat.rootdir * fat.bps, SEEK_SET) < 0) {
    return ERR;
  }

  buf = malloc(fat.spc * fat.bps);
  if (buf == nil) {
    printf("Failed to alloc buf!\n");
    return ENOMEM;
  }

  if (read(fd, buf, fat.spc * fat.bps) < 0) {
    printf("Failed to read root cluster!\n");
    return ERR;
  }

  printf("parse files\n");
  
  file = (struct fat_dir_entry *) buf;
  while ((uint8_t *) file < buf + fat.spc * fat.bps) {
    if (file->attr == 0) {
      break;
    } else if ((file->attr & FAT_ATTR_lfn) == FAT_ATTR_lfn) {
      printf("This is a long file name entry\n");

      lfn = (struct fat_lfn *) file;

      printf("order = %i\n", lfn->order);

    } else {
      printf("have entry name = '%s' ext = '%s' \n", file->name, file->ext);
      printf("with attributes 0b%b\n", file->attr);
      s = intcopylittle32(file->size);
      printf("of size %u\n", s);
    }

    file++;
  }

  printf("finished reading root dir\n");

  free(buf);

  return ERR;
}

uint32_t
clusteroffset(struct fat *fat, uint32_t cluster)
{
  return (fat->dataarea + ((cluster - 2) * fat->spc))
    * fat->bps;
}
