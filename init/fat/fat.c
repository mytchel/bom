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
  uint32_t rootcluster, s;
  uint32_t reservedsectors;
  uint8_t *buf;
  
  if (read(fd, &fat.bs, sizeof(fat.bs)) != sizeof(fat.bs)) {
    printf("Failed to read boot sector.\n");
    return ERR;
  }

  fat.sectorsize = intcopylittle16(fat.bs.sector_size);
  fat.clustersize = fat.bs.cluster_size;

  printf("%i sectors per cluster, %i bytes per sector\n",
	 fat.clustersize, fat.sectorsize);
  
  fat.fatsize = intcopylittle16(fat.bs.sectors_per_fat);
  fat.nfat = fat.bs.nfats;

  reservedsectors = intcopylittle16(fat.bs.reserved_sectors);

  fat.firstsector = reservedsectors +
    fat.nfat * fat.fatsize;

  printf("reserverd sectors = %i\n", reservedsectors);
  printf("fatsize = %i\n", fat.fatsize);
  printf("nfat = %i\n", fat.nfat);
  printf("firstsector at %i\n", fat.firstsector);
  
  s = intcopylittle16(fat.bs.sector_count_16);
  if (s == 0) {
    printf("sector count 16 is zero, read 32\n");
    s = intcopylittle32(fat.bs.sector_count_32);
  }

  printf("there are %i sectors\n", s);

  rootcluster = intcopylittle32(fat.bs.ext.root_cluster);

  printf("rootcluster = %i\n", rootcluster);
  printf("seek to %i\n", clusteroffset(&fat, rootcluster));
  if (seek(fd, clusteroffset(&fat, rootcluster), SEEK_SET) < 0) {
    return ERR;
  }

  buf = malloc(fat.clustersize * fat.sectorsize);
  if (buf == nil) {
    printf("Failed to alloc buf!\n");
    return ENOMEM;
  }

  printf("buf at 0x%h\n", buf);
  printf("read root cluster\n");

  if (read(fd, buf, fat.clustersize * fat.sectorsize) < 0) {
    printf("Failed to read root cluster!\n");
    return ERR;
  }

  printf("parse files\n");
  
  file = (struct fat_dir_entry *) buf;
  while ((uint8_t *) file < buf + fat.clustersize * fat.sectorsize) {
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

  printf("now err\n");
  return ERR;
}

uint32_t
clusteroffset(struct fat *fat, uint32_t cluster)
{
  return (fat->firstsector + ((cluster - 2) * fat->clustersize))
    * fat->sectorsize;
}
