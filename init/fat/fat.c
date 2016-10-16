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
  uint32_t rootcluster, s;
  uint16_t sectorsize;
  uint8_t clustersize;
  uint8_t *buf;
  
  if (read(fd, &fat.bs, sizeof(fat.bs)) != sizeof(fat.bs)) {
    printf("Failed to read boot sector.\n");
    return ERR;
  }

  memmove(&sectorsize, fat.bs.sector_size, sizeof(sectorsize));
  memmove(&clustersize, &fat.bs.cluster_size, sizeof(clustersize));

  printf("%i bytes per sector, %i sectors per cluster\n",
	 sectorsize, clustersize);


  buf = malloc(sizeof(uint8_t) * sectorsize * clustersize);
  if (buf == nil) {
    printf("Failed to allocat buf!\n");
    return ENOMEM;
  }
  
  memmove(&rootcluster, fat.bs.ext.root_cluster,
	  sizeof(fat.bs.ext.root_cluster));

  printf("root cluster at 0x%h\n", rootcluster);

  if (seek(fd, rootcluster, SEEK_SET) < 0) {
    return ERR;
  }

  if (read(fd, buf, sizeof(uint8_t) * sectorsize * clustersize) < 0) {
    printf("Failed to read root cluster!\n");
    return ERR;
  }

  file = (struct fat_dir_entry *) buf;
  while (file->name[0] != 0) {
    printf("have entry '%s'\n", file->name);
    printf("with attributes 0b%b\n", file->attr);
    memmove(&s, file->size, sizeof(file->size));
    printf("of size %u\n", s);

    file++;
  }
  
  return ERR;
}
