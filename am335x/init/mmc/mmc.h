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

enum { RESP_NO, RESP_LEN_48, RESP_LEN_48_CHK_BUSY, RESP_LEN_136 };
enum { DATA_NONE, DATA_READ, DATA_WRITE };

struct mmc_command {
  uint32_t cmd;
  uint32_t arg;
  int resp_type;
  uint32_t resp[4];
  uint8_t *data;
  uint32_t data_len;
  int data_type;
};

enum { MMC_EMMC, MMC_SD };

struct mmc {
  volatile struct mmchs_regs *regs;
  
  uint8_t *data;
  size_t data_len;

  int intr;

  int type;
  uint32_t nblk;

  uint32_t rca;
  uint32_t csd[4];
  uint32_t scr[2];

  char *name;
  int mbrpid;
};

bool
mmchssendcmd(struct mmc *mmc, struct mmc_command *c);

bool
mmchssendappcmd(struct mmc *mmc, struct mmc_command *c);

bool
mmchssoftreset(struct mmc *mmc);

void
mmccmdreset(struct mmc *mmc);

bool
cardgotoidle(struct mmc *mmc);

bool
cardidentification(struct mmc *mmc);

bool
cardqueryvolttype(struct mmc *mmc);

bool
cardidentify(struct mmc *mmc);

bool
cardselect(struct mmc *mmc);

bool
cardcsd(struct mmc *mmc);

bool
mmcinit(struct mmc *mmc);

bool
sdinit(struct mmc *mmc);

bool
readblock(void *aux, uint32_t blk, uint8_t *buf);

bool
writeblock(void *aux, uint32_t blk, uint8_t *buf);

int
__bitfield(uint32_t *src, int start, int len);

#endif
