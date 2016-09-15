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

#include "sdmmcreg.h"
#include "sdhcreg.h"
#include "omap_mmc.h"

#define MMCHS0	0x48060000 /* sd */
#define MMC1	0x481D8000 /* mmc */
#define MMCHS2	0x47810000 /* not used */

#define MMC0_intr 64
#define MMC1_intr 28
#define MMC2_intr 29

struct mmchs_command {
  uint32_t cmd;
  uint32_t arg;
  bool want_resp;
  bool read;
  bool write;
  uint32_t resp;
  uint8_t *data;
  uint32_t data_len;
};

static void
handle_brr(struct mmchs *mmchs);

static void
handle_bwr(struct mmchs *mmchs);

static bool
mmchsintrwait(struct mmchs *mmchs, uint32_t mask)
{
  uint32_t v;

  printf("%s wait intr\nie  = 0b%b\nise = 0b%b\n", mmchs->name,
	 mmchs->regs->ie, mmchs->regs->ise);
  
  while (waitintr(mmchs->intr) != ERR) {
    while ((v = mmchs->regs->stat) != 0) {
      printf("%s intr, stat = 0b%b\n", mmchs->name, v);
      if (v & MMCHS_SD_STAT_BRR) {
	handle_brr(mmchs);
	continue;
      } else if (v & MMCHS_SD_STAT_BWR) {
	handle_bwr(mmchs);
	continue;
      }

      if (v & mask) {
	return true;
      } else if (v & MMCHS_SD_STAT_ERROR_MASK) {
	return false;
      }

      printf("%s unexpected interrupt 0b%b\n", mmchs->name, v);
    }
  }

  return false;
}

void
handle_bwr(struct mmchs *mmchs)
{
  size_t i;
  uint32_t v;

  printf("handle bwr\n");

  if (mmchs->data == nil) {
    printf("%s->data == nil!\n", mmchs->name);
    return;
  }

  for (i = 0; i < mmchs->data_len; i += 4) {
    while (!(mmchs->regs->pstate & MMCHS_SD_PSTATE_BWE)) {
      printf("%s pstate bad!\n", mmchs->name);
    }

    *((uint8_t *) &v) = mmchs->data[i];
    *((uint8_t *) &v + 1) = mmchs->data[i+1];
    *((uint8_t *) &v + 2) = mmchs->data[i+2];
    *((uint8_t *) &v + 3) = mmchs->data[i+3];

    mmchs->regs->data = v;
  }

  mmchs->regs->stat |= MMCHS_SD_STAT_BWR;
  mmchs->data = nil;
}

void
handle_brr(struct mmchs *mmchs)
{
  size_t i;
  uint32_t v;
  
  printf("handle brd\n");

  if (mmchs->data == nil) {
    printf("%s->data == nil!\n", mmchs->name);
    return;
  }

  for (i = 0; i < mmchs->data_len; i += 4) {
    while (!(mmchs->regs->pstate & MMCHS_SD_PSTATE_BRE)) {
      printf("%s pstate bad!\n", mmchs->name);
    }

    v = mmchs->regs->data;

    mmchs->data[i] = *((uint8_t *) &v);
    mmchs->data[i+1] = *((uint8_t *) &v + 1);
    mmchs->data[i+2] = *((uint8_t *) &v + 2);
    mmchs->data[i+3] = *((uint8_t *) &v + 3);
  }

  mmchs->regs->stat |= MMCHS_SD_STAT_BRR;
  mmchs->data = nil;
}

static bool
mmchssendrawcmd(struct mmchs *mmchs, uint32_t cmd, uint32_t arg)
{
  if (mmchs->regs->stat != 0) {
    printf("%s stat in bad shape 0b%b\n",
	   mmchs->name, mmchs->regs->stat);
    return false;
  }

  mmchs->regs->arg = arg;
  mmchs->regs->cmd = cmd;

  if (!mmchsintrwait(mmchs, MMCHS_SD_STAT_CC)) {
    mmchs->regs->stat |= MMCHS_SD_STAT_CC;
    return false;
  }

  mmchs->regs->stat |= MMCHS_SD_STAT_CC;

  return true;
}

static bool
mmchssendcmd(struct mmchs *mmchs, struct mmchs_command *c)
{
  uint32_t cmd;
  bool r;

  cmd = MMCHS_SD_CMD_INDX_CMD(c->cmd);

  if (c->want_resp) {
    cmd |= MMCHS_SD_CMD_RSP_TYPE_48B;
  } else {
    cmd |= MMCHS_SD_CMD_RSP_TYPE_NO_RESP;
  }

  if (c->read || c->write) {
    if (c->data == nil || c->data_len > 0xfff)
      return false;
    
    mmchs->data = c->data;
    mmchs->data_len = c->data_len;

    mmchs->regs->blk &= ~MMCHS_SD_BLK_BLEN;
    mmchs->regs->blk |= c->data_len;

    cmd |= MMCHS_SD_CMD_DP_DATA;
    cmd |= MMCHS_SD_CMD_MSBS_SINGLE;
  }
  
  if (c->read) {
    cmd |= MMCHS_SD_CMD_DDIR_READ;
    mmchs->regs->ie |= MMCHS_SD_IE_BRR_ENABLE;
  } else if (c->write) {
    cmd |= MMCHS_SD_CMD_DDIR_WRITE;
    mmchs->regs->ie |= MMCHS_SD_IE_BWR_ENABLE;
  }

  r = mmchssendrawcmd(mmchs, cmd, c->arg);
  printf("%s back from raw\n", mmchs->name);

  if (c->read || c->write) {
    printf("%s wait for tc\n", mmchs->name);
    if (!mmchsintrwait(mmchs, MMCHS_SD_IE_TC_ENABLE)) {
      printf("%s wait for transfer complete failed!\n", mmchs->name);
      return false;
    }

    mmchs->regs->stat |= MMCHS_SD_IE_TC_ENABLE;
  }
  
  if (c->read) {
    mmchs->regs->ie |= MMCHS_SD_IE_BRR_ENABLE;
  } else if (c->write) {
    mmchs->regs->ie |= MMCHS_SD_IE_BWR_ENABLE;
  }

  if (c->want_resp) {
    c->resp = mmchs->regs->rsp10;
  }
  
  return r;
}

static bool
mmchsinit(struct mmchs *mmchs)
{
  int i;

  printf("%s rev 0b%b (%i)\n", mmchs->name,
	 mmchs->regs->rev, mmchs->regs->rev);

  mmchs->regs->sysconfig |= MMCHS_SD_SYSCONFIG_SOFTRESET;

  i = 100;
  do {
    sleep(1);
    i--;
  } while (i > 0 &&
	   !(mmchs->regs->sysstatus & MMCHS_SD_SYSSTATUS_RESETDONE));

  if (!i) {
    printf("%s: Failed to reset controller!\n", mmchs->name);
    return false;
  }

  /* Enable 1.8, 3.0 3.3V */
  mmchs->regs->capa |= MMCHS_SD_CAPA_VS18 |
    MMCHS_SD_CAPA_VS30 |
    MMCHS_SD_CAPA_VS33;

  mmchs->regs->sysconfig |= MMCHS_SD_SYSCONFIG_AUTOIDLE;
  mmchs->regs->sysconfig |= MMCHS_SD_SYSCONFIG_ENAWAKEUP;
  mmchs->regs->sysconfig &= ~MMCHS_SD_SYSCONFIG_SIDLEMODE;
  mmchs->regs->sysconfig |= MMCHS_SD_SYSCONFIG_SIDLEMODE_IDLE;
  mmchs->regs->sysconfig &= ~MMCHS_SD_SYSCONFIG_CLOCKACTIVITY;
  mmchs->regs->sysconfig |= MMCHS_SD_SYSCONFIG_CLOCKACTIVITY_OFF;
  mmchs->regs->sysconfig &= ~MMCHS_SD_SYSCONFIG_STANDBYMODE;
  mmchs->regs->sysconfig |= MMCHS_SD_SYSCONFIG_STANDBYMODE_WAKEUP_INTERNAL;

  /* wakup on sd interrupt for sdio */
  mmchs->regs->hctl |= MMCHS_SD_HCTL_IWE;

  /* change to 1 bit mode */ 
  mmchs->regs->con &= ~MMCHS_SD_CON_DW8;
  mmchs->regs->hctl &= ~MMCHS_SD_HCTL_DTW;

  /* set to 3.0V */
  mmchs->regs->hctl &= ~MMCHS_SD_HCTL_SDVS;
  mmchs->regs->hctl |= MMCHS_SD_HCTL_SDVS_VS30;

  /* Power on card */
  mmchs->regs->hctl |= MMCHS_SD_HCTL_SDBP;

  i = 100;
  do {
    sleep(1);
    i--;
  } while (i > 0 && !(mmchs->regs->hctl & MMCHS_SD_HCTL_SDBP));

  if (!i) {
    printf("%s: failed to power on card!\n", mmchs->name);
    return false;
  }

  /* enable internal clock and clock to card */
  mmchs->regs->sysctl |= MMCHS_SD_SYSCTL_ICE;

  /* set clock frequence to 400kHz */
  mmchs->regs->sysctl &= ~MMCHS_SD_SYSCTL_CLKD;
  mmchs->regs->sysctl |= 0xf0 << 6;

  /* enable clock */
  mmchs->regs->sysctl |= MMCHS_SD_SYSCTL_CEN;

  i = 100;
  do {
    sleep(1);
    i--;
  } while (i > 0 && !(mmchs->regs->sysctl & MMCHS_SD_SYSCTL_ICS));

  if (!i) {
    printf("%s: mmchs clock not stable\n", mmchs->name);
    return false;
  }

  /* Enable interrupts */
  mmchs->regs->ie |= MMCHS_SD_IE_CC_ENABLE;
  mmchs->regs->ie |= MMCHS_SD_IE_TC_ENABLE;
  mmchs->regs->ie |= MMCHS_SD_IE_ERROR_MASK;

  /* clear stat register */
  mmchs->regs->stat = 0xffffffff;

  /* send init signal */
  mmchs->regs->con |= MMCHS_SD_CON_INIT;
  mmchs->regs->cmd = 0;

  i = 25;
  do {
    sleep(1);
    i--;
  } while (i > 0 && !(mmchs->regs->stat & MMCHS_SD_STAT_CC));

  if (!i) {
    printf("%s: mmchs command 0 failed!\n", mmchs->name);
    return false;
  }

  /* clean stat */
  mmchs->regs->stat |= MMCHS_SD_IE_CC_ENABLE;

  /* remove con init */
  mmchs->regs->con &= ~MMCHS_SD_CON_INIT;
 
  /* enable send interrupts to intc */
  mmchs->regs->ise = 0xffffffff;
  
  return true;
}

static bool
mmchsgotoidle(struct mmchs *mmchs)
{
  struct mmchs_command cmd;

  cmd.cmd = MMC_GO_IDLE_STATE;
  cmd.want_resp = false;
  cmd.read = cmd.write = false;
  cmd.arg = 0;
  cmd.data = nil;

  return mmchssendcmd(mmchs, &cmd);
}

/*
static bool
mmchsidentification(struct mmchs *mmchs)
{
  struct mmchs_command cmd;

  cmd.cmd = SD_SEND_IF_COND;
  cmd.want_resp = true;
  cmd.read = cmd.write = false;
  cmd.data = nil;
  cmd.arg = MMCHS_SD_ARG_CMD8_VHS | MMCHS_SD_ARG_CMD8_CHECK_PATTERN;

  if (!mmchssendcmd(mmchs, &cmd)) {
    printf("%s identification command failed!\n", mmchs->name);
    return false;
  }

  if (cmd.resp !=
      (MMCHS_SD_ARG_CMD8_VHS | MMCHS_SD_ARG_CMD8_CHECK_PATTERN)) {
    printf("%s check pattern check failed 0x%h\n",
	   mmchs->name, cmd.resp);
    return false;
  }

  return true;
}
*/
static bool
readblock(struct mmchs *mmchs, uint32_t blk, uint8_t *buf)
{
  struct mmchs_command cmd;

  cmd.cmd = MMC_READ_BLOCK_SINGLE;
  cmd.arg = blk;
  cmd.want_resp = true;
  cmd.read = true;
  cmd.write = false;
  cmd.data = buf;
  cmd.data_len = 512;

  return mmchssendcmd(mmchs, &cmd);
}

/*
static bool
writeblock(struct mmchs *mmchs, uint32_t blk, uint32_t *buf)
{
  struct mmchs_command cmd;

  cmd.cmd = MMC_WRITE_BLOCK_SINGLE;
  cmd.want_resp = true;
  cmd.read = false;
  cmd.write = true;
  cmd.arg = blk;
  cmd.data = buf;
  cmd.data_len = 512;

  return mmchssendcmd(mmchs, &cmd);
}
*/

static int
mmchsproc(char *name, void *addr, int intr)
{
  struct mmchs *mmchs;
  size_t size = 4 * 1024;

  printf("%s running in proc %i\n", name, getpid());
  
  mmchs = malloc(sizeof(struct mmchs));
  if (!mmchs) {
    printf("%s malloc failed!\n", name);
    return -1;
  }

  mmchs->intr = intr;
  mmchs->name = name;
  mmchs->regs = (struct mmchs_regs *) getmem(MEM_io, addr, &size);
  if (!mmchs->regs) {
    printf("%s failed to map register space!\n", mmchs->name);
    return -2;
  }

  if (!mmchsinit(mmchs)) {
    printf("%s failed to init!\n", mmchs->name);
    rmmem((void *) mmchs->regs, size);
    return -2;
  }

  if (!mmchsgotoidle(mmchs)) {
    printf("%s failed to goto idle state!\n", mmchs->name);
    return -3;
  }
  /*  
  if (!mmchsidentification(mmchs)) {
    printf("%s failed to do identification, possibly mmc card!\n",
	   mmchs->name);
    return -4;
  }

  printf("%s identification good\n", mmchs->name);
  */

  printf("%s try read a block\n", mmchs->name);
  
  uint32_t i;
  uint8_t buf[512];
  if (!readblock(mmchs, 1, buf)) {
    printf("%s failed to read stat = 0b%b\n",
	   mmchs->name, mmchs->regs->stat);
    return -5;
  }

  printf("%s print block 1\n", mmchs->name);

  for (i = 0; i < 512; i += 4)
    printf("%s %i %h %h %h %h\n", mmchs->name,
	   buf[i], buf[i+1], buf[i+2], buf[i+3]);
  
  return 0;
}

int
mmc(void)
{
  int p;
	
  printf("Should init MMCHS cards\n");

  p = fork(FORK_sngroup);
  if (p == 0) {
    return mmchsproc("mmc0", (void *) MMCHS0, MMC0_intr);
  } else if (p < 0) {
    printf("mmchs failed to fork for mmc0\n");
  }

  p = fork(FORK_sngroup);
  if (p == 0) {
    return mmchsproc("mmc1", (void *) MMC1, MMC1_intr);
  } else if (p < 0) {
    printf("mmchs failed to fork for mmc1\n");
  }

  return 0;
}

