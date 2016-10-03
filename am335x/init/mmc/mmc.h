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

#ifndef _MMC_H_
#define _MMC_H_

enum { RESP_NO, RESP_LEN_48, RESP_LEN_136 };

struct mmc_command {
  uint32_t cmd;
  uint32_t arg;
  int resp_type;
  uint32_t resp[4];
  bool read, write;
  uint8_t *data;
  uint32_t data_len;
};

struct mmc {
  volatile struct mmchs_regs *regs;
  
  uint8_t *data;
  size_t data_len;

  int intr;
  
  uint32_t rca;
  uint32_t csd[4];
  uint32_t nblk;

  char *name;
  int mbrpid;
};

bool
mmchssendcmd(struct mmc *mmc, struct mmc_command *c);

bool
readblock(void *aux, uint32_t blk, uint8_t *buf);

bool
writeblock(void *aux, uint32_t blk, uint8_t *buf);

#endif
