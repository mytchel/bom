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
mmcproc(void *addr, int intr);

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
    printf("mmc failed to fork for mmc0\n");
  } else if (p > 0) {
    return mmcproc((void *) MMCHS0, MMC0_intr);
  }

  p = fork(FORK_sngroup);
  if (p == 0) {
    printf("mmc failed to fork for mmc1\n");
  } else if (p > 0) {
    return mmcproc((void *) MMC1, MMC1_intr);
  }

  p = fork(FORK_sngroup);
  if (p == 0) {
    printf("mmc failed to fork for mmc2\n");
  } else if (p > 0) {
    return mmcproc((void *) MMCHS2, MMC2_intr);
  }

  return 0;
}

int
mmcproc(void *addr, int intr)
{
  struct mmc *mmc;
  size_t size = 4 * 1024;

  mmc = malloc(sizeof(struct mmc));
  if (!mmc) {
    printf("mmc malloc failed!\n");
    return -1;
  }

  mmc->intr = intr;
  
  mmc->regs = (struct mmc_regs *) getmem(MEM_io, addr, &size);
  if (!mmc->regs) {
    printf("mmc failed to map register space!\n");
    return -2;
  }

  printf("%i: init mmc\n", getpid());
  if (mmcinit(mmc)) {
    printf("failed to init mmc!\n");
    rmmem((void *) mmc->regs, size);
    return -2;
  }

  printf("%i: finished setting up mmc card as best as I could.\n",
	 getpid());

  mmcintrwait(mmc, 0xffffffff);

  return 0;
}

int
mmcintrwait(struct mmc *mmc, uint32_t mask)
{
  uint32_t v;
  
  printf("%i: waiting for intr\n", getpid());
  while (waitintr(mmc->intr) != ERR) {
    printf("%i: got intr %i, 0b%b\n", getpid(), mmc->intr, v);
    while ((v = mmc->regs->sd_stat) != 0) {
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

      printf("mmc unexpected interrupt 0b%b\n", v);
    }
  }

  return 1;
}

int
mmcinit(struct mmc *mmc)
{
  int i;

  printf("mmc status = 0b%b\n", mmc->regs->sysstatus);
  printf("mmc config = 0b%b\n", mmc->regs->sysconfig);
  printf("mmc pstate = 0b%b\n", mmc->regs->sd_pstate);

  mmc->regs->sysconfig |= MMCHS_SD_SYSCONFIG_SOFTRESET;

  i = 25;
  do {
    sleep(0);
    i--;
  } while (i > 0 &&
	   !(mmc->regs->sysstatus & MMCHS_SD_SYSSTATUS_RESETDONE));

  if (!i) {
    printf("%i: Failed to reset controller!\n", getpid());
    return -1;
  }

  printf("mmc card reset (took %i loops)\n", i);
  printf("mmc pstate = 0b%b\n", mmc->regs->sd_pstate);

  printf("set capa\n");
  /* Enable 1.8, 3.0, 3.3V */
  mmc->regs->capa |= MMCHS_SD_CAPA_VS18 | MMCHS_SD_CAPA_VS30;

  printf("set sysconfig\n");
  
  mmc->regs->sysconfig |= MMCHS_SD_SYSCONFIG_AUTOIDLE;
  mmc->regs->sysconfig |= MMCHS_SD_SYSCONFIG_ENAWAKEUP;
  mmc->regs->sysconfig &= ~MMCHS_SD_SYSCONFIG_SIDLEMODE;
  mmc->regs->sysconfig |= MMCHS_SD_SYSCONFIG_SIDLEMODE_IDLE;
  mmc->regs->sysconfig &= ~MMCHS_SD_SYSCONFIG_CLOCKACTIVITY;
  mmc->regs->sysconfig |= MMCHS_SD_SYSCONFIG_CLOCKACTIVITY_OFF;
  mmc->regs->sysconfig &= ~MMCHS_SD_SYSCONFIG_STANDBYMODE;
  mmc->regs->sysconfig |= MMCHS_SD_SYSCONFIG_STANDBYMODE_WAKEUP_INTERNAL;

  printf("wakup on sd interrupt for sdio\n");

  mmc->regs->hctl = MMCHS_SD_HCTL_IWE;

  printf("set 3v\n");
  /* set to 3.0V */
  mmc->regs->hctl &= ~MMCHS_SD_HCTL_SDVS;
  mmc->regs->hctl |= MMCHS_SD_HCTL_SDVS_VS30;

  printf("%i: hctl = 0b%b\n", getpid(), mmc->regs->hctl);

  printf("change to 1 bit mode\n");
  /* change to 1 bit mode */ 
  mmc->regs->sd_con &= ~MMCHS_SD_CON_DW8;
  mmc->regs->hctl &= ~MMCHS_SD_HCTL_DTW;

  printf("power on card\n");
  /* Power on card */
  mmc->regs->hctl |= MMCHS_SD_HCTL_SDBP;

  i = 25;
  do {
    sleep(0);
    i--;
  } while (!(mmc->regs->hctl & (1 << 8)) && i > 0);

  if (!i) {
    printf("%i: failed to power on card!\n", getpid());
    return -2;
  }

  printf("card on\n");
  
  /* clear stat register */
  mmc->regs->sd_stat = 0xffffffff;

  /* enable interrupts */
  mmc->regs->sd_ise = 0xfffffff;
  
  printf("mmc pstate = 0b%b\n", mmc->regs->sd_pstate);

  return 0;
}

void
handle_bwr(struct mmc *mmc)
{
  printf("handle bwr\n");

  if (!(mmc->regs->sd_pstate & MMCHS_SD_PSTATE_BWE)) {
    printf("pstate bad for write\n");
    return;
  }

  mmc->regs->sd_stat |= MMCHS_SD_STAT_BWR;
}

void
handle_brr(struct mmc *mmc)
{
  printf("handle brd\n");

  if (!(mmc->regs->sd_pstate & MMCHS_SD_PSTATE_BRE)) {
    printf("pstate bad for read\n");
    return;
  }

  mmc->regs->sd_stat |= MMCHS_SD_STAT_BRR;
}
