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
#include "mmchs.h"

#define MMCHS0	0x48060000 /* sd */
#define MMC1	0x481D8000 /* mmc */
#define MMCHS2	0x47810000 /* not used */

#define MMC0_intr 64
#define MMC1_intr 28
#define MMC2_intr 29

void
mmcreset(struct mmc *mmc, uint32_t line)
{
  int i;

  mmc->regs->stat = 0xffffffff;
  mmc->regs->sysctl |= line;

  i = 100;
  while (!(mmc->regs->sysctl & line) && i-- > 0)
    ;

  if (i == 0) {
    printf("%s reset timeout waiting for set\n", mmc->name);
  }

  i = 10000;
  while ((mmc->regs->sysctl & line) && i-- > 0)
    ;

  if (i == 0) {
    printf("%s reset timeout waiting for clear\n", mmc->name);
  }
}

bool
mmchssendcmd(struct mmc *mmc, uint32_t cmd, uint32_t arg)
{
  uint32_t stat;
  
  if (mmc->regs->stat != 0) {
    printf("%s stat bad for cmd, 0b%b\n",
	   mmc->name, mmc->regs->stat);

    return false;
  }

  mmc->regs->arg = arg;
  mmc->regs->cmd = cmd;

  if (waitintr(mmc->intr) != OK) {
    return false;
  }

  stat = mmc->regs->stat;

  mmc->regs->stat = MMCHS_SD_STAT_CC
    | MMCHS_SD_STAT_CIE
    | MMCHS_SD_STAT_CCRC
    | MMCHS_SD_STAT_CTO;

  if (stat & MMCHS_SD_STAT_CTO) {
    mmcreset(mmc, MMCHS_SD_SYSCTL_SRC);
    return false;
  } else {
    return true;
  }
}

bool
mmchssendappcmd(struct mmc *mmc, uint32_t cmd, uint32_t arg)
{
  uint32_t acmd;

  acmd = MMCHS_SD_CMD_INDEX_CMD(MMC_APP_CMD)
    | MMCHS_SD_CMD_RSP_TYPE_48B;

  if (!mmchssendcmd(mmc, acmd, MMC_ARG_RCA(mmc->rca))) {
    return false;
  }

  return mmchssendcmd(mmc, cmd, arg);
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
  mmc->regs->ie = 0
    | MMCHS_SD_STAT_DCRC
    | MMCHS_SD_STAT_DTO
    | MMCHS_SD_STAT_DEB
    | MMCHS_SD_STAT_CIE
    | MMCHS_SD_STAT_CCRC
    | MMCHS_SD_STAT_CTO
    | MMCHS_SD_IE_CC
    | MMCHS_SD_IE_TC
    | MMCHS_SD_IE_BRR
    | MMCHS_SD_IE_BWR;

  /* enable send interrupts to intc */
  mmc->regs->ise = 0
    | MMCHS_SD_STAT_DCRC
    | MMCHS_SD_STAT_DTO
    | MMCHS_SD_STAT_DEB
    | MMCHS_SD_STAT_CIE
    | MMCHS_SD_STAT_CCRC
    | MMCHS_SD_STAT_CTO
    | MMCHS_SD_IE_CC
    | MMCHS_SD_IE_TC
    | MMCHS_SD_IE_BRR
    | MMCHS_SD_IE_BWR;

  return true;
}

static bool
mmchsinitstream(struct mmc *mmc)
{
  /* send init signal */
  mmc->regs->con |= MMCHS_SD_CON_INIT;

  mmchssendcmd(mmc, 0, 0);

  /* remove init */
  mmc->regs->con &= ~MMCHS_SD_CON_INIT;

  mmc->regs->stat = 0xffffffff;

  return true;
}

bool
cardgotoidle(struct mmc *mmc)
{
  uint32_t cmd;

  cmd = MMCHS_SD_CMD_INDEX_CMD(MMC_GO_IDLE_STATE)
    | MMCHS_SD_CMD_RSP_TYPE_NO_RESP;

  return mmchssendcmd(mmc, cmd, 0);
}

bool
cardidentification(struct mmc *mmc)
{
  uint32_t cmd, arg;

  cmd = MMCHS_SD_CMD_INDEX_CMD(SD_SEND_IF_COND)
    | MMCHS_SD_CMD_RSP_TYPE_48B;

  arg = MMCHS_SD_ARG_CMD8_VHS | MMCHS_SD_ARG_CMD8_CHECK_PATTERN;

  if (!mmchssendcmd(mmc, cmd, arg)) {
    return false;
  }

  if (mmc->regs->rsp10 != arg) {
    printf("%s check pattern check failed 0x%h\n",
	   mmc->name, mmc->regs->rsp10);
    return false;
  } else {
    return true;
  }
}

bool
cardqueryvolttype(struct mmc *mmc)
{
  uint32_t cmd, arg;

  cmd = MMCHS_SD_CMD_INDEX_CMD(SD_APP_OP_COND)
    | MMCHS_SD_CMD_RSP_TYPE_48B;

  arg =
    MMC_OCR_3_3V_3_4V | MMC_OCR_3_2V_3_3V | MMC_OCR_3_1V_3_2V |
    MMC_OCR_3_0V_3_1V | MMC_OCR_2_9V_3_0V | MMC_OCR_2_8V_2_9V |
    MMC_OCR_2_7V_2_8V | MMC_OCR_HCS;

  if (mmchssendappcmd(mmc, cmd, arg)) {
    mmc->type = MMC_SD;

    while (!(mmc->regs->rsp10 & MMC_OCR_MEM_READY)) {
      if (!mmchssendappcmd(mmc, cmd, arg)) {
	return false;
      }
    }
  } else {
    mmc->type = MMC_EMMC;
    
    mmcreset(mmc, MMCHS_SD_SYSCTL_SRC);

    cmd = MMCHS_SD_CMD_INDEX_CMD(MMC_SEND_OP_COND)
      | MMCHS_SD_CMD_RSP_TYPE_48B;

    do {
      if (!mmchssendcmd(mmc, cmd, arg)) {
	return false;
      }
    } while (!(mmc->regs->rsp10 & MMC_OCR_MEM_READY));
  }

  return true;
}

bool
cardidentify(struct mmc *mmc)
{
  uint32_t cmd;

  cmd = MMCHS_SD_CMD_INDEX_CMD(MMC_ALL_SEND_CID)
    | MMCHS_SD_CMD_RSP_TYPE_136B;

  if (!mmchssendcmd(mmc, cmd, 0)) {
    printf("%s send cid failed\n", mmc->name);
    return false;
  }

  cmd = MMCHS_SD_CMD_INDEX_CMD(MMC_SET_RELATIVE_ADDR)
    | MMCHS_SD_CMD_RSP_TYPE_48B;

  if (!mmchssendcmd(mmc, cmd, 0)) {
    printf("%s set relative addr failed\n", mmc->name);
    return false;
  }

  mmc->rca = mmc->regs->rsp10 >> 16;

  return true;
}

bool
cardselect(struct mmc *mmc)
{
  uint32_t cmd, stat;

  cmd = MMCHS_SD_CMD_INDEX_CMD(MMC_SELECT_CARD)
    | MMCHS_SD_CMD_RSP_TYPE_48B_BUSY;

  if (!mmchssendcmd(mmc, cmd, MMC_ARG_RCA(mmc->rca))) {
    return false;
  }


  if (waitintr(mmc->intr) != OK) {
    return false;
  }

  stat = mmc->regs->stat;
  mmc->regs->stat = MMCHS_SD_STAT_TC
    | MMCHS_SD_STAT_DTO
    | MMCHS_SD_STAT_DCRC;

  if (stat & MMCHS_SD_STAT_ERRI) {
    mmcreset(mmc, MMCHS_SD_SYSCTL_SRD);
    return false;
  } else {
    return true;
  }
}

bool
cardcsd(struct mmc *mmc)
{
  uint32_t cmd;

  cmd = MMCHS_SD_CMD_INDEX_CMD(MMC_SEND_CSD)
    | MMCHS_SD_CMD_RSP_TYPE_136B;

  if (!mmchssendcmd(mmc, cmd, MMC_ARG_RCA(mmc->rca))) {
    return false;
  }

  mmc->csd[0] = mmc->regs->rsp10;
  mmc->csd[1] = mmc->regs->rsp32;
  mmc->csd[2] = mmc->regs->rsp54;
  mmc->csd[3] = mmc->regs->rsp76;

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
    mmcreset(mmc, MMCHS_SD_SYSCTL_SRC);
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
mmchsreaddata(struct mmc *mmc, uint32_t *buf, size_t l)
{
  uint32_t stat;
  
  if (waitintr(mmc->intr) != OK) {
    return false;
  }

  if (mmc->regs->stat & MMCHS_SD_STAT_BRR) {
    mmc->regs->stat = MMCHS_SD_STAT_BRR;

    while (l > 0) {
      *buf++ = mmc->regs->data;
      l -= sizeof(uint32_t);
    }
  }

  if (waitintr(mmc->intr) != OK) {
    return false;
  }

  stat = mmc->regs->stat;
  mmc->regs->stat = MMCHS_SD_STAT_TC
    | MMCHS_SD_STAT_DCRC
    | MMCHS_SD_STAT_DTO
    | MMCHS_SD_STAT_DEB;

  if (stat & MMCHS_SD_STAT_ERRI) {
    mmcreset(mmc, MMCHS_SD_SYSCTL_SRD);
    return false;
  } else {
    return true;
  }
}

bool
mmchswritedata(struct mmc *mmc, uint32_t *buf, size_t l)
{
  uint32_t stat;
  
  if (waitintr(mmc->intr) != OK) {
    return false;
  }

  if (mmc->regs->stat & MMCHS_SD_STAT_BWR) {
    printf("%s got bwr\n", mmc->name);
    mmc->regs->stat = MMCHS_SD_STAT_BWR;

    while (l > 0) {
      mmc->regs->data = *buf++;
      l -= sizeof(uint32_t);
    }
  }

  printf("%s wait for tc\n", mmc->name);
  if (waitintr(mmc->intr) != OK) {
    return false;
  }

  printf("%s got intr\n", mmc->name);
  stat = mmc->regs->stat;
  mmc->regs->stat = MMCHS_SD_STAT_TC
    | MMCHS_SD_STAT_DCRC
    | MMCHS_SD_STAT_DTO
    | MMCHS_SD_STAT_DEB;

  if (stat & MMCHS_SD_STAT_ERRI) {
    printf("%s write data error waiting for tc, stat = 0b%b\n",
	   mmc->name, stat);

    mmcreset(mmc, MMCHS_SD_SYSCTL_SRD);
    return false;
  } else {
    return true;
  }
}

bool
readblock(void *aux, uint32_t blk, uint8_t *buf)
{
  struct mmc *mmc = (struct mmc *) aux;
  uint32_t cmd;

  printf("%s read %i\n", mmc->name, blk);
  
  cmd = MMCHS_SD_CMD_INDEX_CMD(MMC_READ_BLOCK_SINGLE)
    | MMCHS_SD_CMD_RSP_TYPE_48B
    | MMCHS_SD_CMD_DP_DATA
    | MMCHS_SD_CMD_CICE_ENABLE
    | MMCHS_SD_CMD_CCCE_ENABLE
    | MMCHS_SD_CMD_DDIR_READ;

  mmc->regs->blk = 512;

  if (!mmchssendcmd(mmc, cmd, blk)) {
    return false;
  } 

  return mmchsreaddata(mmc, (uint32_t *) buf, 512);
}

bool
writeblock(void *aux, uint32_t blk, uint8_t *buf)
{
  struct mmc *mmc = (struct mmc *) aux;
  uint32_t cmd;

  printf("%s write %i\n", mmc->name, blk);

  cmd = MMCHS_SD_CMD_INDEX_CMD(MMC_WRITE_BLOCK_SINGLE)
    | MMCHS_SD_CMD_RSP_TYPE_48B
    | MMCHS_SD_CMD_DP_DATA
    | MMCHS_SD_CMD_CICE_ENABLE
    | MMCHS_SD_CMD_CCCE_ENABLE
    | MMCHS_SD_CMD_DDIR_WRITE;

  mmc->regs->blk = 512;

  if (!mmchssendcmd(mmc, cmd, blk)) {
    mmc->regs->stat = MMCHS_SD_STAT_ERROR_MASK;
    return false;
  } 

  return mmchswritedata(mmc, (uint32_t *) buf, 512);
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

