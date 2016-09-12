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

#include "libc.h"
#include "init/mmc.h"

#define MMCHS0	0x48060000 /* sd */
#define MMC1	0x481D8000 /* mmc */
#define MMCHS2	0x47810000 /* not used */

#define MMC0_intr 64
#define MMC1_intr 28
#define MMC2_intr 29

static int
mmcinit(struct mmc *);

static int
mmcproc(char *name, void *addr, int intr);

static int
mmcintrwait(struct mmc *mmc, uint32_t mask);

static void
handle_brr(struct mmc *mmc);

static void
handle_bwr(struct mmc *mmc);

int
mmc(void)
{
  int p;
	
  printf("Should init MMC cards\n");

  p = fork(FORK_sngroup);
  if (p == 0) {
    return mmcproc("mmc0", (void *) MMCHS0, MMC0_intr);
  } else if (p < 0) {
    printf("mmc failed to fork for mmc0\n");
  }

  p = fork(FORK_sngroup);
  if (p == 0) {
    return mmcproc("mmc1", (void *) MMC1, MMC1_intr);
  } else if (p < 0) {
    printf("mmc failed to fork for mmc1\n");
  }

  /* Not supported i believe
   *
   */
  /*
  p = fork(FORK_sngroup);
  if (p == 0) {
    return mmcproc("mmc2", (void *) MMCHS2, MMC2_intr);
  } else if (p < 0) {
    printf("mmc failed to fork for mmc2\n");
  }
  */
  
  return 0;
}

int
mmcproc(char *name, void *addr, int intr)
{
  struct mmc *mmc;
  size_t size = 4 * 1024;

  printf("%s running in proc %i\n", name, getpid());
  
  mmc = malloc(sizeof(struct mmc));
  if (!mmc) {
    printf("%s malloc failed!\n", name);
    return -1;
  }

  mmc->intr = intr;
  mmc->name = name;
  mmc->regs = (struct mmc_regs *) getmem(MEM_io, addr, &size);
  if (!mmc->regs) {
    printf("%s failed to map register space!\n", mmc->name);
    return -2;
  }

  if (mmcinit(mmc)) {
    printf("%s failed to init!\n", mmc->name);
    rmmem((void *) mmc->regs, size);
    return -2;
  }

  printf("%s finished setting up mmc card as best as I could.\n",
	 mmc->name);

  mmcintrwait(mmc, 0xffffffff);

  return 0;
}

int
mmcintrwait(struct mmc *mmc, uint32_t mask)
{
  uint32_t v;
  
  printf("%s: waiting for intr\n", mmc->name);
  while (waitintr(mmc->intr) != ERR) {
    printf("%s: got intr %i, 0b%b\n", mmc->name, mmc->intr, v);
    while ((v = mmc->regs->stat) != 0) {
      if (v & MMCHS_SD_STAT_BRR) {
	handle_brr(mmc);
	continue;
      } else if (v & MMCHS_SD_STAT_BWR) {
	handle_bwr(mmc);
	continue;
      }

      if (v & mask) {
	return 0;
      } else if (v & MMCHS_SD_STAT_ERROR_MASK) {
	return 1;
      }

      printf("%s unexpected interrupt 0b%b\n", mmc->name, v);
    }
  }

  return 1;
}

int
mmcinit(struct mmc *mmc)
{
  int i;

  printf("%s rev 0b%b (%i)\n", mmc->name,
	 mmc->regs->rev, mmc->regs->rev);

  mmc->regs->sysconfig |= MMCHS_SD_SYSCONFIG_SOFTRESET;

  i = 100;
  do {
    sleep(1);
    i--;
  } while (i > 0 &&
	   !(mmc->regs->sysstatus & MMCHS_SD_SYSSTATUS_RESETDONE));

  if (!i) {
    printf("%s: Failed to reset controller!\n", mmc->name);
    return -1;
  }

  /* Enable 1.8, 3.0 3.3V */
  mmc->regs->capa |= MMCHS_SD_CAPA_VS18 |
    MMCHS_SD_CAPA_VS30 |
    MMCHS_SD_CAPA_VS33;

  mmc->regs->sysconfig |= MMCHS_SD_SYSCONFIG_AUTOIDLE;
  mmc->regs->sysconfig |= MMCHS_SD_SYSCONFIG_ENAWAKEUP;
  mmc->regs->sysconfig &= ~MMCHS_SD_SYSCONFIG_SIDLEMODE;
  mmc->regs->sysconfig |= MMCHS_SD_SYSCONFIG_SIDLEMODE_IDLE;
  mmc->regs->sysconfig &= ~MMCHS_SD_SYSCONFIG_CLOCKACTIVITY;
  mmc->regs->sysconfig |= MMCHS_SD_SYSCONFIG_CLOCKACTIVITY_OFF;
  mmc->regs->sysconfig &= ~MMCHS_SD_SYSCONFIG_STANDBYMODE;
  mmc->regs->sysconfig |= MMCHS_SD_SYSCONFIG_STANDBYMODE_WAKEUP_INTERNAL;

  /* wakup on sd interrupt for sdio */
  mmc->regs->hctl |= MMCHS_SD_HCTL_IWE;

  /* change to 1 bit mode */ 
  mmc->regs->con &= ~MMCHS_SD_CON_DW8;
  mmc->regs->hctl &= ~MMCHS_SD_HCTL_DTW;

  /* set to 3.0V */
  mmc->regs->hctl &= ~MMCHS_SD_HCTL_SDVS;
  mmc->regs->hctl |= MMCHS_SD_HCTL_SDVS_VS30;

  /* Power on card */
  mmc->regs->hctl |= MMCHS_SD_HCTL_SDBP;

  i = 100;
  do {
    sleep(1);
    i--;
  } while (i > 0 && !(mmc->regs->hctl & MMCHS_SD_HCTL_SDBP));

  if (!i) {
    printf("%s: failed to power on card!\n", mmc->name);
    return -2;
  }

  printf("%s powered on!\n", mmc->name);

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
    printf("%s: mmc clock not stable\n", mmc->name);
    return -3;
  }

  /* Enable interrupts */
  mmc->regs->ie |= MMCHS_SD_IE_CC_ENABLE;
  mmc->regs->ie |= MMCHS_SD_IE_TC_ENABLE;
  mmc->regs->ie |= MMCHS_SD_IE_ERROR_MASK;

  /* clear stat register */
  mmc->regs->stat = 0xffffffff;

  printf("%s send init\n", mmc->name);
  /* send init signal */
  mmc->regs->con |= MMCHS_SD_CON_INIT;
  mmc->regs->cmd = 0;

  i = 25;
  do {
    sleep(0);
    i--;
  } while (i > 0 && !(mmc->regs->stat & MMCHS_SD_STAT_CC));

  if (!i) {
    printf("%s: mmc command 0 failed!\n", mmc->name);
    return -2;
  }

  /* remove con init */
  mmc->regs->con &= ~MMCHS_SD_CON_INIT;

  /* clean stat */
  mmc->regs->stat |= MMCHS_SD_IE_CC_ENABLE;
  
  /* enable send interrupts to intc */
  mmc->regs->ise = 0xfffffff;
  
  return 0;
}

void
handle_bwr(struct mmc *mmc)
{
  printf("handle bwr\n");

  if (!(mmc->regs->pstate & MMCHS_SD_PSTATE_BWE)) {
    printf("pstate bad for write\n");
    return;
  }

  mmc->regs->stat |= MMCHS_SD_STAT_BWR;
}

void
handle_brr(struct mmc *mmc)
{
  printf("handle brd\n");

  if (!(mmc->regs->pstate & MMCHS_SD_PSTATE_BRE)) {
    printf("pstate bad for read\n");
    return;
  }

  mmc->regs->stat |= MMCHS_SD_STAT_BRR;
}
