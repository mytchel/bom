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

static void
handle_brr(struct mmc *);

static void
handle_bwr(struct mmc *);

int
__bitfield(uint32_t *src, int start, int len);

static bool
mmchswaitintr(struct mmc *mmc, uint32_t mask)
{
  uint32_t v;

  v = mmc->regs->ie;
  mmc->regs->ie = v;
  mmc->regs->ie = 0xffffffff;

  while (waitintr(mmc->intr) != ERR) {
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
  }

  return false;
}

void
handle_bwr(struct mmc *mmc)
{
  size_t i;
  uint32_t v;

  if (mmc->data == nil) {
    printf("%s->data == nil!\n", mmc->name);
    return;
  } else if (!(mmc->regs->pstate & MMCHS_SD_PSTATE_BWE)) {
    printf("%s pstate bad!\n", mmc->name);
    return;
  } 

  for (i = 0; i < mmc->data_len; i += 4) {
    *((uint8_t *) &v) = mmc->data[i];
    *((uint8_t *) &v + 1) = mmc->data[i+1];
    *((uint8_t *) &v + 2) = mmc->data[i+2];
    *((uint8_t *) &v + 3) = mmc->data[i+3];

    mmc->regs->data = v;
  }

  mmc->regs->stat = MMCHS_SD_STAT_BWR;
  mmc->data = nil;
}

void
handle_brr(struct mmc *mmc)
{
  size_t i;
  uint32_t v;

  if (mmc->data == nil) {
    printf("%s->data == nil!\n", mmc->name);
    return;
  } else if (!(mmc->regs->pstate & MMCHS_SD_PSTATE_BRE)) {
    printf("%s pstate bad!\n", mmc->name);
    return;
  }

  for (i = 0; i < mmc->data_len; i += 4) {
    v = mmc->regs->data;

    mmc->data[i] = *((uint8_t *) &v);
    mmc->data[i+1] = *((uint8_t *) &v + 1);
    mmc->data[i+2] = *((uint8_t *) &v + 2);
    mmc->data[i+3] = *((uint8_t *) &v + 3);
  }

  mmc->regs->stat = MMCHS_SD_STAT_BRR;
  mmc->data = nil;
}

static void
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
  sleep(0);
  mmc->regs->cmd = cmd;

  if (!mmchswaitintr(mmc, MMCHS_SD_STAT_CC)) {
    mmc->regs->stat = MMCHS_SD_STAT_CC;
    return false;
  }

  mmc->regs->stat = MMCHS_SD_STAT_CC;

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
  case RESP_LEN_136:
    cmd |= MMCHS_SD_CMD_RSP_TYPE_136B;
    break;
  case RESP_NO:
    cmd |= MMCHS_SD_CMD_RSP_TYPE_NO_RESP;
    break;
  default:
    return false;
  }

  if (c->read || c->write) {
    if (c->data == nil || c->data_len > 0xfff)
      return false;
    
    mmc->data = c->data;
    mmc->data_len = c->data_len;

    mmc->regs->blk &= ~MMCHS_SD_BLK_BLEN;
    mmc->regs->blk |= c->data_len;

    cmd |= MMCHS_SD_CMD_DP_DATA;
    cmd |= MMCHS_SD_CMD_MSBS_SINGLE;
  }
  
  if (c->read) {
    cmd |= MMCHS_SD_CMD_DDIR_READ;
  } else if (c->write) {
    cmd |= MMCHS_SD_CMD_DDIR_WRITE;
  }

  r = mmchssendrawcmd(mmc, cmd, c->arg);

  if (r && (c->read || c->write)) {
    if (!mmchswaitintr(mmc, MMCHS_SD_IE_TC)) {
      printf("%s wait for transfer complete failed!\n",
	     mmc->name);
      return false;
    }

    mmc->regs->stat = MMCHS_SD_IE_TC;
  }
  
  switch (c->resp_type) {
  case RESP_LEN_48:
    c->resp[0] = mmc->regs->rsp10;
  case RESP_LEN_136:
    c->resp[1] = mmc->regs->rsp32;
    c->resp[2] = mmc->regs->rsp54;
    c->resp[3] = mmc->regs->rsp76;
    break;
  case RESP_NO:
    break;
  }

  return r;
}

static bool
mmchssendappcmd(struct mmc *mmc, struct mmc_command *c)
{
  struct mmc_command cmd;

  cmd.cmd = MMC_APP_CMD;
  cmd.arg = mmc->rca;
  cmd.resp_type = RESP_LEN_48;
  cmd.read = false; cmd.write = false;
  cmd.data = nil;
  cmd.data_len = 0;

  if (!mmchssendcmd(mmc, &cmd)) {
    return false;
  }

  return mmchssendcmd(mmc, c);
}

static bool
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

static bool
cardgotoidle(struct mmc *mmc)
{
  struct mmc_command cmd;

  cmd.cmd = MMC_GO_IDLE_STATE;
  cmd.resp_type = RESP_NO;
  cmd.read = cmd.write = false;
  cmd.arg = 0;
  cmd.data = nil;

  return mmchssendcmd(mmc, &cmd);
}

static bool
cardidentification(struct mmc *mmc)
{
  struct mmc_command cmd;

  cmd.cmd = SD_SEND_IF_COND;
  cmd.resp_type = RESP_LEN_48;
  cmd.read = cmd.write = false;
  cmd.data = nil;
  cmd.arg = MMCHS_SD_ARG_CMD8_VHS | MMCHS_SD_ARG_CMD8_CHECK_PATTERN;

  if (!mmchssendcmd(mmc, &cmd)) {
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

static bool
cardqueryvolttype(struct mmc *mmc)
{
  struct mmc_command cmd;
  int r;

  cmd.cmd = SD_APP_OP_COND;
  cmd.resp_type = RESP_LEN_48;
  cmd.read = cmd.write = false;
  cmd.data = nil;
  cmd.arg =
    MMC_OCR_3_3V_3_4V | MMC_OCR_3_2V_3_3V | MMC_OCR_3_1V_3_2V |
    MMC_OCR_3_0V_3_1V | MMC_OCR_2_9V_3_0V | MMC_OCR_2_8V_2_9V |
    MMC_OCR_2_7V_2_8V | MMC_OCR_HCS;

  r = mmchssendappcmd(mmc, &cmd);

  if (!r && (mmc->regs->stat & MMCHS_SD_STAT_CTO)) {
    /* eMMC cards here. */
    printf("%s is a mmc card\n", mmc->name);
    mmccmdreset(mmc);

    cmd.cmd = MMC_SEND_OP_COND;
    while (!(cmd.resp[0] & MMC_OCR_MEM_READY)) {
      if (!mmchssendcmd(mmc, &cmd)) {
	return false;
      }
    }
   
  } else if (r) {
    /* SD cards here. */
    printf("%s is a sd card\n", mmc->name);
    while (!(cmd.resp[0] & MMC_OCR_MEM_READY)) {
      if (!mmchssendappcmd(mmc, &cmd)) {
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

static bool
cardidentify(struct mmc *mmc)
{
  struct mmc_command cmd;

  cmd.cmd = MMC_ALL_SEND_CID;
  cmd.resp_type = RESP_LEN_136;
  cmd.read = cmd.write = false;
  cmd.data = nil;
  cmd.arg = 0;

  if (!mmchssendcmd(mmc, &cmd)) {
    return false;
  }

  cmd.cmd = MMC_SET_RELATIVE_ADDR;
  cmd.resp_type = RESP_LEN_48;

  if (!mmchssendcmd(mmc, &cmd)) {
    return false;
  }

  mmc->rca = SD_R6_RCA(cmd.resp);

  return true;
}

static bool
cardselect(struct mmc *mmc)
{
  struct mmc_command cmd;
  
  cmd.cmd = MMC_SELECT_CARD;
  cmd.resp_type = RESP_LEN_48;
  cmd.read = cmd.write = false;
  cmd.data = nil;
  cmd.arg = MMC_ARG_RCA(mmc->rca);

  return mmchssendcmd(mmc, &cmd);
}

static bool
cardcsd(struct mmc *mmc)
{
  struct mmc_command cmd;

  cmd.cmd = MMC_SEND_CSD;
  cmd.resp_type = RESP_LEN_136;
  cmd.read = cmd.write = false;
  cmd.data = nil;
  cmd.arg = MMC_ARG_RCA(mmc->rca);

  if (!mmchssendcmd(mmc, &cmd)) {
    printf("%s csd failed, stat = 0b%b\n",
	   mmc->name, mmc->regs->stat);
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

  if (SD_CSD_READ_BL_LEN(mmc->csd) != 0x09) {
    printf("%s block length is not 512!\n", mmc->name);
    return false;
  }

  return true;
}

bool
readblock(void *aux, uint32_t blk, uint8_t *buf)
{
  struct mmc_command cmd;

  cmd.cmd = MMC_READ_BLOCK_SINGLE;
  cmd.arg = blk;
  cmd.resp_type = RESP_LEN_48;
  cmd.read = true;
  cmd.write = false;
  cmd.data = buf;
  cmd.data_len = 512;

  return mmchssendcmd((struct mmc *) aux, &cmd);
}

bool
writeblock(void *aux, uint32_t blk, uint8_t *buf)
{
  struct mmc_command cmd;

  cmd.cmd = MMC_WRITE_BLOCK_SINGLE;
  cmd.resp_type = RESP_LEN_48;
  cmd.read = false;
  cmd.write = true;
  cmd.arg = blk;
  cmd.data = buf;
  cmd.data_len = 512;

  return mmchssendcmd((struct mmc *) aux, &cmd);
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
    return -2;
  }

  if (!mmchssoftreset(&mmc)) {
    printf("%s failed to reset host!\n", mmc.name);
    return -2;
  }
  
  if (!mmchsinit(&mmc)) {
    printf("%s failed to init host!\n", mmc.name);
    return -2;
  }

  if (!mmchsinitstream(&mmc)) {
    printf("%s failed to init host stream!\n", mmc.name);
    return -2;
  }

  if (!cardinit(&mmc)) {
    printf("%s failed to init card!\n", mmc.name);
    return -3;
  }

  switch (SD_CSD_CSDVER(mmc.csd)) {
  case 0:
    mmc.nblk = SD_CSD_CAPACITY(mmc.csd);
    break;
  case 2:
    mmc.nblk = SD_CSD_V2_CAPACITY(mmc.csd);
    break;
  case 3:
    mmc.nblk = MMC_CSD_CAPACITY(mmc.csd);
    break;
  default:
    printf("%s has unknown csd version %i\n", mmc.name,
	   SD_CSD_CSDVER(mmc.csd));
  }

  device.name = name;

  device.read = &readblock;
  device.write = &writeblock;
  device.nblk = mmc.nblk;
  device.aux = &mmc;

  mmc.mbrpid = mbrmountthread(&device, (uint8_t *) "/dev/");
  if (mmc.mbrpid < 0) {
    printf("mbr exited!\n");
    return mmc.mbrpid;
  } else {
    while (true)
      sleep(100);
  }

  return 0;
}

int
initblockdevs(void)
{
  int p;

  p = fork(FORK_sngroup);
  if (p == 0) {
    return mmchsproc("mmc0", (void *) MMCHS0, MMC0_intr);
  } else if (p < 0) {
    printf("Failed to fork for mmc0\n");
  }

  p = fork(FORK_sngroup);
  if (p == 0) {
    return mmchsproc("mmc1", (void *) MMC1, MMC1_intr);
  } else if (p < 0) {
    printf("Failed to fork for mmc1\n");
  }

  return 0;
}

