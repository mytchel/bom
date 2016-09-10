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

#include "../include/libc.h"

#define MMCHS0	0x48060000 /* sd */
#define MMC1	0x481D8000 /* mmc */
#define MMCHS2	0x47810000 /* not used */

#define MMC0_intr 64
#define MMC1_intr 28
#define MMC2_intr 29

struct mmc_regs {
  uint32_t pad[68];
  uint32_t sysconfig;
  uint32_t sysstatus;
  uint32_t pad1[3];
  uint32_t csre;
  uint32_t systest;
  uint32_t con;
  uint32_t pwcnt;
  uint32_t pad2[52];
  uint32_t sdmasa;
  uint32_t blk;
  uint32_t arg;
  uint32_t cmd;
  uint32_t rsp10;
  uint32_t rsp32;
  uint32_t rsp54;
  uint32_t rsp76;
  uint32_t data;
  uint32_t sd_pstate;
  uint32_t hctl;
  uint32_t sysctl;
  uint32_t sd_stat;
  uint32_t sd_ie;
  uint32_t sd_ise;
  uint32_t ac12;
  uint32_t capa;
  uint32_t pad3[1];
  uint32_t curcapa;
  uint32_t pad4[2];
  uint32_t fe;
  uint32_t admaes;
  uint32_t admasal;
  uint32_t admasah;
  uint32_t pad5[40];
  uint32_t rev;
};

struct mmc {
  volatile struct mmc_regs *regs;
  int intr;
};

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
      if (v & (1 << 5)) {
	handle_brr(mmc);
	continue;
      } else if (v & (1 << 4)) {
	handle_bwr(mmc);
	continue;
      }

      if (v & mask) {
	return 0;
      } else if (v & (1 << 15)) {
	/* Error */
	return 1;
      }

      printf("mmc unexspected intrrupt 0b%b\n", v);
    }
  }

  return 1;
}

int
mmcinit(struct mmc *mmc)
{
  int i;
  uint32_t t;

  printf("mmc status = 0b%b\n", mmc->regs->sysstatus);
  printf("mmc config = 0b%b\n", mmc->regs->sysconfig);
  printf("mmc pstate = 0b%b\n", mmc->regs->sd_pstate);

  mmc->regs->sysconfig |= 1<<1; /* softreset */

  i = 25;
  do {
    sleep(0);
    i--;
  } while (!(mmc->regs->sysstatus & 1) && i > 0);

  if (!i) {
    printf("%i: Failed to reset controller!\n", getpid());
    return -1;
  }

  printf("mmc card reset (took %i loops)\n", i);
  printf("mmc pstate = 0b%b\n", mmc->regs->sd_pstate);

  /* Enable 1.8, 3.0, 3.3V */
  mmc->regs->capa |= 1 << 26 | 1 << 25 | 1 << 24;

  printf("enable wakeup interrupt\n");
  
  /* set to 3.0V */
  t = mmc->regs->hctl;
  t &= 7 << 9;
  t |= 0x6 << 9;
  /* enable wake up interrupt */
  t |= 1 << 24;
  mmc->regs->hctl = t;

  printf("%i: hctl = 0b%b\n", getpid(), mmc->regs->hctl);

  /* change to 1 bit mode */ 
  mmc->regs->con &= ~(1 << 5);
  mmc->regs->hctl &= ~(1 << 1);
  
  /* Power on card */
  mmc->regs->hctl |= 1 << 8;

  i = 25;
  do {
    sleep(0);
    i--;
  } while (!(mmc->regs->hctl & (1 << 8)) && i > 0);

  if (!i) {
    printf("%i: failed to power on card!\n", getpid());
    return -2;
  }

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

  if (!(mmc->regs->sd_pstate & (1 << 10))) {
    printf("pstate bad for write\n");
    return;
  }

  mmc->regs->sd_stat |= 1 << 4;
}

void
handle_brr(struct mmc *mmc)
{
  printf("handle brd\n");

  if (!(mmc->regs->sd_pstate & (1 << 11))) {
    printf("pstate bad for read\n");
    return;
  }

  mmc->regs->sd_stat |= 1 << 5;
}
