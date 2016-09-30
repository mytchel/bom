/*
 *   Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <libc.h>
#include <fs.h>
#include <stdarg.h>
#include <string.h>

#include "mbr.h"

struct partition {
  uint8_t status;
  uint8_t start_chs[3];
  uint8_t type;
  uint8_t end_chs[3];
  uint32_t lba;
  uint32_t sectors;
} __attribute__ ((__packed__));

struct mbr {
  uint8_t bootstrap[446];
  struct partition parts[4];
  uint8_t bootsignature[2];
};

static char name[256];
static int device;

static struct mbr mbr;

static bool
readmbr(void)
{
  uint32_t lba;
  uint32_t sectors;
  int i;
  
  if (seek(device, 0, SEEK_SET) != OK) {
    printf("seek failed\n");
    return false;
  }

  if (read(device, &mbr, sizeof(mbr)) != sizeof(mbr)) {
    printf("read failed\n");
    return false;
  } else {

    for (i = 0; i < 4; i++) {
      printf("%s part%i status = 0b%b\n", name, i, mbr.parts[i].status);
      printf("%s part%i type = 0x%h\n", name, i, mbr.parts[i].type);

      memmove(&lba, &mbr.parts[i].lba, sizeof(uint32_t));
      memmove(&sectors, &mbr.parts[i].sectors, sizeof(uint32_t));

      printf("%s part%i start at 0x%h\n", name, i, lba);
      printf("%s part%i has %i sectors\n", name, i, sectors);
    }
    
    return true;
  }
}

static void
bopen(struct request *req, struct response *resp)
{
  printf("%s opened\n", name);
  resp->ret = OK;
}

static void
bclose(struct request *req, struct response *resp)
{
  printf("%s close\n", name);
  resp->ret = OK;
}

static void
bwalk(struct request *req, struct response *resp)
{
  printf("%s walk\n", name);
  resp->ret = ENOIMPL;
}

static void
bread(struct request *req, struct response *resp)
{
  resp->ret = ENOIMPL;
}

static void
bwrite(struct request *req, struct response *resp)
{
  resp->ret = ENOIMPL;
}

static void
bremove(struct request *req, struct response *resp)
{
  printf("%s remove, should exit.\n", name);
  resp->ret = ENOIMPL;
}

static struct fsmount mount = {
  &bopen,
  &bclose,
  &bwalk,
  &bread,
  &bwrite,
  &bremove,
  nil,
};

int
mbrmount(char *devicepath, char *nname)
{
  int p1[2], p2[3], fd, i;
  char filename[256];

  memmove(name, nname, strlen(nname));
  
  for (i = 0; i < 10; i++) {
    device = open(devicepath, O_RDWR);
    if (device > 0) {
      break;
    }
  }

  if (device < 0) {
    printf("%s failed to open %s\n", name, devicepath);
    exit(-1);
  }

  if (!readmbr()) {
    printf("%s failed to read mbr\n", name);
    exit(-2);
  }
  
  snprintf(filename, sizeof(filename), "/dev/%s", name);

  fd = open(filename, O_WRONLY|O_CREATE, ATTR_dir|ATTR_rd);
  if (fd < 0) {
    printf("%s failed to create %s.\n", name, filename);
    return -1;
  }

  if (pipe(p1) == ERR) {
    return -2;
  } else if (pipe(p2) == ERR) {
    return -2;
  }

  if (bind(p1[1], p2[0], filename) == ERR) {
    return -3;
  }

  close(p1[1]);
  close(p2[0]);

  return fsmountloop(p1[0], p2[1], &mount);
}

