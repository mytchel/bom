/*
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
