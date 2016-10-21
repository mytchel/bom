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

#include <libc.h>
#include <mem.h>
#include <fs.h>
#include <stdarg.h>
#include <string.h>
#include <block.h>

#include "sdmmcreg.h"
#include "sdhcreg.h"
#include "omap_mmc.h"
#include "mmc.h"

#define MMCHS0	0x48060000 /* sd */
#define MMC1	0x481D8000 /* mmc */
#define MMCHS2	0x47810000 /* not used */

#define MMC0_intr 64
#define MMC1_intr 28
#define MMC2_intr 29

/* Error bits in the card status (R1) response. */
#define R1_ERROR_MASK 0xFDFFA080

static void
handle_bwr(struct mmc *mmc)
{
  size_t i;
  uint32_t v;

  for (i = 0; i < mmc->data_len; i += 4) {
    *((uint8_t *) &v) = mmc->data[i];
    *((uint8_t *) &v + 1) = mmc->data[i+1];
    *((uint8_t *) &v + 2) = mmc->data[i+2];
    *((uint8_t *) &v + 3) = mmc->data[i+3];

    mmc->regs->data = v;
  }

  mmc->regs->stat = MMCHS_SD_IE_BWR;
}

static void
handle_brr(struct mmc *mmc)
{
  size_t i;
  uint32_t v;

  for (i = 0; i < mmc->data_len; i += 4) {
    v = mmc->regs->data;

    mmc->data[i] = *((uint8_t *) &v);
    mmc->data[i+1] = *((uint8_t *) &v + 1);
    mmc->data[i+2] = *((uint8_t *) &v + 2);
    mmc->data[i+3] = *((uint8_t *) &v + 3);
  }

  mmc->regs->stat = MMCHS_SD_IE_BRR;
}

static bool
mmchswaitintr(struct mmc *mmc, uint32_t mask)
{
  uint32_t v;

  do {
    while ((v = mmc->regs->stat) != 0) {
      if (v & MMCHS_SD_STAT_BRR) {
	handle_brr(mmc);
	continue;
      } else if (v & MMCHS_SD_STAT_BWR) {
	handle_bwr(mmc);
	continue;
      }

      if (v & mask) {
	return true;
      } else if (v & MMCHS_SD_STAT_ERROR_MASK) {
	return false;
      }

      printf("%s unexpected interrupt 0b%b\n", mmc->name, v);
    }
  } while (waitintr(mmc->intr) != ERR);

  return false;
}

void
mmccmdreset(struct mmc *mmc)
{
  int i = 100;
  
  mmc->regs->stat = 0xffffffff;
  mmc->regs->sysctl |= MMCHS_SD_SYSCTL_SRC;

  while (!(mmc->regs->sysctl & MMCHS_SD_SYSCTL_SRC) && i-- > 0);

  i = 100;
  while ((mmc->regs->sysctl & MMCHS_SD_SYSCTL_SRC) && i-- > 0)
    sleep(1);
}

static bool
mmchssendrawcmd(struct mmc *mmc, uint32_t cmd, uint32_t arg)
{
  if (mmc->regs->stat != 0) {
    printf("%s stat in bad shape 0b%b\n",
	   mmc->name, mmc->regs->stat);
    return false;
  }

  mmc->regs->arg = arg;
  mmc->regs->cmd = cmd;

  if (!mmchswaitintr(mmc, MMCHS_SD_STAT_CC)) {
    mmc->regs->stat = MMCHS_SD_STAT_CC;
    return false;
  }

  mmc->regs->stat = MMCHS_SD_STAT_CC;

  if ((cmd & MMCHS_SD_CMD_RSP_TYPE) ==
      MMCHS_SD_CMD_RSP_TYPE_48B_BUSY) {

    if (!mmchswaitintr(mmc, MMCHS_SD_STAT_TC)) {
      mmc->regs->stat = MMCHS_SD_STAT_TC;
      return false;
    }

    mmc->regs->stat = MMCHS_SD_STAT_TC;
  }

  return true;
}

bool
mmchssendcmd(struct mmc *mmc, struct mmc_command *c)
{
  uint32_t cmd;
  bool r;

  cmd = MMCHS_SD_CMD_INDX_CMD(c->cmd);

  switch (c->resp_type) {
  case RESP_LEN_48:
    cmd |= MMCHS_SD_CMD_RSP_TYPE_48B;
    break;
  case RESP_LEN_48_CHK_BUSY:
    cmd |= MMCHS_SD_CMD_RSP_TYPE_48B_BUSY;
    break;
   case RESP_LEN_136:
    cmd |= MMCHS_SD_CMD_RSP_TYPE_136B;
    break;
  case RESP_NO:
    cmd |= MMCHS_SD_CMD_RSP_TYPE_NO_RESP;
    break;
  default:
    return false;
  }

  if (c->data_type != DATA_NONE) {
    if (c->data == nil) {
      return false;
    }
    
    mmc->data = c->data;
    mmc->data_len = c->data_len;
    mmc->regs->blk = c->data_len;

    cmd |= MMCHS_SD_CMD_DP_DATA;
  }
  
  if (c->data_type == DATA_READ) {
    cmd |= MMCHS_SD_CMD_DDIR_READ;
  } else if (c->data_type == DATA_WRITE) {
    cmd |= MMCHS_SD_CMD_DDIR_WRITE;
  }

  r = mmchssendrawcmd(mmc, cmd, c->arg);

  if (!r) {
    return false;
  } else if (c->data_type != DATA_NONE) {
    if (!mmchswaitintr(mmc, MMCHS_SD_IE_TC)) {
      printf("%s wait for transfer complete failed!\n",
	     mmc->name);
      return false;
    }
    
    mmc->regs->stat = MMCHS_SD_IE_TC;
  }
  
  switch (c->resp_type) {
  case RESP_LEN_48_CHK_BUSY:
  case RESP_LEN_48:
    c->resp[0] = mmc->regs->rsp10;
    break;
  case RESP_LEN_136:
    c->resp[0] = mmc->regs->rsp10;
    c->resp[1] = mmc->regs->rsp32;
    c->resp[2] = mmc->regs->rsp54;
    c->resp[3] = mmc->regs->rsp76;
    break;
  case RESP_NO:
    break;
  }

  return true;
}

bool
mmchssendappcmd(struct mmc *mmc, struct mmc_command *c)
{
  struct mmc_command cmd;

  cmd.cmd = MMC_APP_CMD;
  cmd.resp_type = RESP_LEN_48;
  cmd.data_type = DATA_NONE;
  cmd.arg = MMC_ARG_RCA(mmc->rca);

  if (!mmchssendcmd(mmc, &cmd)) {
    return false;
  }

  return mmchssendcmd(mmc, c);
}

bool
mmchssoftreset(struct mmc *mmc)
{
  int i = 100;

  mmc->regs->sysconfig |= MMCHS_SD_SYSCONFIG_SOFTRESET;

  while (!(mmc->regs->sysstatus & MMCHS_SD_SYSSTATUS_RESETDONE)) {
    sleep(1);
    if (i-- < 0) {
      printf("%s: Failed to reset controller!\n", mmc->name);
      return false;
    }
  }

  return true;
}

static bool
mmchsinit(struct mmc *mmc)
{
  int i;

  /* 1-bit mode */
  mmc->regs->con &= ~MMCHS_SD_CON_DW8;

  /* set to 3.0V */
  mmc->regs->hctl = MMCHS_SD_HCTL_SDVS_VS30;
  mmc->regs->capa |= MMCHS_SD_CAPA_VS18 | MMCHS_SD_CAPA_VS30;

  /* Power on card */
  mmc->regs->hctl |= MMCHS_SD_HCTL_SDBP;

  i = 100;
  while (!(mmc->regs->hctl & MMCHS_SD_HCTL_SDBP)) {
    sleep(1);
    if (i-- < 0) {
      printf("%s: failed to power on card!\n", mmc->name);
      return false;
    }
  }

  /* enable internal clock and clock to card */
  mmc->regs->sysctl |= MMCHS_SD_SYSCTL_ICE;

  /* set clock frequence to 400kHz */
  mmc->regs->sysctl &= ~MMCHS_SD_SYSCTL_CLKD;
  mmc->regs->sysctl |= 0xf0 << 6;

  /* enable clock */
  mmc->regs->sysctl |= MMCHS_SD_SYSCTL_CEN;

  i = 100;
  do {
    sleep(1);
    i--;
  } while (i > 0 && !(mmc->regs->sysctl & MMCHS_SD_SYSCTL_ICS));

  if (!i) {
    printf("%s: mmchs clock not stable\n", mmc->name);
    return false;
  }

  /* Enable interrupts */
  mmc->regs->ie =
    MMCHS_SD_IE_ERROR_MASK |
    MMCHS_SD_IE_CC | MMCHS_SD_IE_TC |
    MMCHS_SD_IE_BRR | MMCHS_SD_IE_BWR;

  /* enable send interrupts to intc */
  mmc->regs->ise = 0xffffffff;

  return true;
}

static bool
mmchsinitstream(struct mmc *mmc)
{
  /* send init signal */
  mmc->regs->con |= MMCHS_SD_CON_INIT;

  mmc->regs->cmd = 0;
  if (!mmchswaitintr(mmc, MMCHS_SD_STAT_CC)) {
    printf("%s failed to send CMD0 (1)\n", mmc->name);
    return false;
  }

  mmc->regs->stat = MMCHS_SD_STAT_CC;

  /* remove init */
  mmc->regs->con &= ~MMCHS_SD_CON_INIT;

  mmc->regs->stat = 0xffffffff;

  return true;
}

bool
cardgotoidle(struct mmc *mmc)
{
  struct mmc_command cmd;

  cmd.cmd = MMC_GO_IDLE_STATE;
  cmd.resp_type = RESP_NO;
  cmd.data_type = DATA_NONE;
  cmd.data = nil;

  if (mmchssendcmd(mmc, &cmd)) {
    return true;
  } else {
    mmc->regs->stat = 0xffffffff;
    return false;
  }
}

bool
cardidentification(struct mmc *mmc)
{
  struct mmc_command cmd;

  cmd.cmd = SD_SEND_IF_COND;
  cmd.resp_type = RESP_LEN_48;
  cmd.data_type = DATA_NONE;
  cmd.arg = MMCHS_SD_ARG_CMD8_VHS | MMCHS_SD_ARG_CMD8_CHECK_PATTERN;

  if (!mmchssendcmd(mmc, &cmd)) {
    mmc->regs->stat = 0xffffffff;
    return false;
  }

  if (cmd.resp[0] !=
      (MMCHS_SD_ARG_CMD8_VHS | MMCHS_SD_ARG_CMD8_CHECK_PATTERN)) {
    printf("%s check pattern check failed 0x%h\n",
	   mmc->name, cmd.resp);
    return false;
  }

  return true;
}

bool
cardqueryvolttype(struct mmc *mmc)
{
  struct mmc_command cmd;
  int r;

  cmd.cmd = SD_APP_OP_COND;
  cmd.resp_type = RESP_LEN_48;
  cmd.data_type = DATA_NONE;
  cmd.arg =
    MMC_OCR_3_3V_3_4V | MMC_OCR_3_2V_3_3V | MMC_OCR_3_1V_3_2V |
    MMC_OCR_3_0V_3_1V | MMC_OCR_2_9V_3_0V | MMC_OCR_2_8V_2_9V |
    MMC_OCR_2_7V_2_8V | MMC_OCR_HCS;

  r = mmchssendappcmd(mmc, &cmd);

  if (r) {
    mmc->type = MMC_SD;

    while (!(cmd.resp[0] & MMC_OCR_MEM_READY)) {
      if (!mmchssendappcmd(mmc, &cmd)) {
	return false;
      }
    }
  } else if (mmc->regs->stat & MMCHS_SD_STAT_CTO) {
    mmc->type = MMC_EMMC;
    
    mmccmdreset(mmc);

    cmd.cmd = MMC_SEND_OP_COND;
    while (!(cmd.resp[0] & MMC_OCR_MEM_READY)) {
      if (!mmchssendcmd(mmc, &cmd)) {
	return false;
      }
    }
  } else {
    printf("%s failed to determine card type for voltage query.\n",
	   mmc->name);
    return false;
  }

  return true;
}

bool
cardidentify(struct mmc *mmc)
{
  struct mmc_command cmd;

  cmd.cmd = MMC_ALL_SEND_CID;
  cmd.resp_type = RESP_LEN_136;
  cmd.data_type = DATA_NONE;
  cmd.arg = 0;

  if (!mmchssendcmd(mmc, &cmd)) {
    return false;
  }

  cmd.cmd = MMC_SET_RELATIVE_ADDR;
  cmd.data_type = DATA_NONE;
  cmd.resp_type = RESP_LEN_48;
  cmd.arg = MMC_ARG_RCA(0);

  if (!mmchssendcmd(mmc, &cmd)) {
    return false;
  }

  mmc->rca = SD_R6_RCA(cmd.resp);

  return true;
}

bool
cardselect(struct mmc *mmc)
{
  struct mmc_command cmd;
  
  cmd.cmd = MMC_SELECT_CARD;
  cmd.resp_type = RESP_LEN_48_CHK_BUSY;
  cmd.data_type = DATA_NONE;
  cmd.arg = MMC_ARG_RCA(mmc->rca);

  return mmchssendcmd(mmc, &cmd);
}

bool
cardcsd(struct mmc *mmc)
{
  struct mmc_command cmd;

  cmd.cmd = MMC_SEND_CSD;
  cmd.resp_type = RESP_LEN_136;
  cmd.data_type = DATA_NONE;
  cmd.arg = MMC_ARG_RCA(mmc->rca);

  if (!mmchssendcmd(mmc, &cmd)) {
    return false;
  }

  mmc->csd[0] = cmd.resp[0];
  mmc->csd[1] = cmd.resp[1];
  mmc->csd[2] = cmd.resp[2];
  mmc->csd[3] = cmd.resp[3];

  return true;
}

static bool
cardinit(struct mmc *mmc)
{
  mmc->rca = 0;
  
  if (!cardgotoidle(mmc)) {
    printf("%s failed to goto idle state!\n", mmc->name);
    return false;
  }

  if (!cardidentification(mmc)) {
    mmccmdreset(mmc);
  }

  if (!cardqueryvolttype(mmc)) {
    printf("%s failed to query voltage and type!\n", mmc->name);
    return false;
  }

  if (!cardidentify(mmc)) {
    printf("%s identify card failed!\n", mmc->name);
    return false;
  }

  if (!cardcsd(mmc)) {
    printf("%s get card csd failed!\n", mmc->name);
    return false;
  }

  if (!cardselect(mmc)) {
    printf("%s select card failed!\n", mmc->name);
    return false;
  }

  return true;
}

bool
readblock(void *aux, uint32_t blk, uint8_t *buf)
{
  struct mmc_command cmd;
  struct mmc *mmc = (struct mmc *) aux;

  cmd.cmd = MMC_READ_BLOCK_SINGLE;
  cmd.arg = blk;
  cmd.resp_type = RESP_LEN_48;
  cmd.data_type = DATA_READ;
  cmd.data = buf;
  cmd.data_len = 512;

  if (!mmchssendcmd(mmc, &cmd)) {
    mmc->regs->stat = 0xffffffff;
    return false;
  } else {
    return true;
  }
}

bool
writeblock(void *aux, uint32_t blk, uint8_t *buf)
{
  struct mmc_command cmd;
  struct mmc *mmc = (struct mmc *) aux;

  cmd.cmd = MMC_WRITE_BLOCK_SINGLE;
  cmd.resp_type = RESP_LEN_48;
  cmd.data_type = DATA_WRITE;
  cmd.arg = blk;
  cmd.data = buf;
  cmd.data_len = 512;

  if (!mmchssendcmd(aux, &cmd)) {
    mmc->regs->stat = 0xffffffff;
    return false;
  } else {
    return true;
  }
}

static int
mmchsproc(char *name, void *addr, int intr)
{
  size_t regsize = 4 * 1024;
  struct blkdevice device;
  struct mmc mmc;

  mmc.name = name;
  mmc.intr = intr;

  mmc.regs = (struct mmchs_regs *) getmem(MEM_io, addr, &regsize);
  if (mmc.regs == nil) {
    printf("%s failed to map register space!\n", mmc.name);
    return ERR;
  }

  if (!mmchssoftreset(&mmc)) {
    printf("%s failed to reset host!\n", mmc.name);
    return ERR;
  }
  
  if (!mmchsinit(&mmc)) {
    printf("%s failed to init host!\n", mmc.name);
    return ERR;
  }

  if (!mmchsinitstream(&mmc)) {
    printf("%s failed to init host stream!\n", mmc.name);
    return ERR;
  }

  if (!cardinit(&mmc)) {
    printf("%s failed to init card!\n", mmc.name);
    return ERR;
  }
  
  if (mmc.type == MMC_EMMC) {
    if (!mmcinit(&mmc)) {
      printf("%s failed to initialise\n", mmc.name);
      return ERR;
    }
  } else if (mmc.type == MMC_SD) {
    if (!sdinit(&mmc)) {
      printf("%s failed to initialise\n", mmc.name);
      return ERR;
    }
  } else {
    printf("%s is of unknown type.\n", mmc.name);
    return ERR;
  }
  
  device.name = mmc.name;
  device.read = &readblock;
  device.write = &writeblock;
  device.nblk = mmc.nblk;
  device.blksize = 512;
  device.aux = &mmc;

  return mbrmount(&device, (uint8_t *) "/dev/");
}

int
initblockdevs(void)
{
  int p;

  p = fork(FORK_sngroup);
  if (p == 0) {
    p = mmchsproc("mmc0", (void *) MMCHS0, MMC0_intr);
    exit(p);
  } else if (p < 0) {
    printf("Failed to fork for mmc0\n");
  }

  p = fork(FORK_sngroup);
  if (p == 0) {
    p = mmchsproc("mmc1", (void *) MMC1, MMC1_intr);
    exit(p);
  } else if (p < 0) {
    printf("Failed to fork for mmc1\n");
  }

  return 0;
}

