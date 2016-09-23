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

#include "head.h"
#include "fns.h"

#define INTC			0x48200000

#define INTC_SYSCONFIG		0x10
#define INTC_SYSSTATUS		0x14
#define INTC_SIR_IRQ		0x40
#define INTC_SIR_FIQ		0x44
#define INTC_CONTROL		0x48
#define INTC_IDLE		0x50

#define INTC_ITRn(i)		0x80+(0x20*i)+0x00
#define INTC_MIRn(i)		0x80+(0x20*i)+0x04
#define INTC_CLEARn(i)		0x80+(0x20*i)+0x08
#define INTC_SETn(i)		0x80+(0x20*i)+0x0c
#define INTC_ISR_SETn(i)	0x80+(0x20*i)+0x10
#define INTC_ISR_CLEARn(i)	0x80+(0x20*i)+0x14
#define INTC_PENDING_IRQn(i)	0x80+(0x20*i)+0x18
#define INTC_PENDING_FIQn(i)	0x80+(0x20*i)+0x1c

#define INTC_ILRn(i)		0x100+(0x04*i)

#define nirq 128

static void
maskintr(int irqn);
static void
unmaskintr(int irqn);

static bool (*handlers[nirq])(uint32_t) = {0};
static struct proc *intrwait = nil;

void
initintc(void)
{
  int i;
  size_t s = 0x1000;

  getpages(&iopages, (void *) INTC, &s);
	
  /* enable interface auto idle */
  writel(1, INTC + INTC_SYSCONFIG);

  /* mask all interrupts. */
  for (i = 0; i < nirq / 32; i++) {
    writel(0xffffffff, INTC + INTC_MIRn(i));
  }
	
  /* Set all interrupts to lowest priority. */
  for (i = 0; i < nirq; i++) {
    writel(63 << 2, INTC + INTC_ILRn(i));
  }
	
  writel(1, INTC + INTC_CONTROL);
}

void
maskintr(int irqn)
{
  uint32_t mask, mfield;

  mfield = irqn / 32;
  mask = 1 << (irqn % 32);

  writel(mask, INTC + INTC_SETn(mfield));
}

void
unmaskintr(int irqn)
{
  uint32_t mask, mfield;

  mfield = irqn / 32;
  mask = 1 << (irqn % 32);

  writel(mask, INTC + INTC_CLEARn(mfield));
}

void
intcaddhandler(uint32_t irqn, bool (*func)(uint32_t))
{
  handlers[irqn] = func;
  unmaskintr(irqn);
}

bool
procwaitintr(struct proc *p, int irqn)
{
  struct proc *pp;
  
  if (irqn < 0 || irqn > nirq) {
    return false;
  } else if (handlers[irqn] != nil) {
    return false;
  }

  for (pp = intrwait; pp != nil; pp = pp->next)
    if (pp->waiting.intr == irqn)
      return false;
  
  p->waiting.intr = irqn;
  p->wnext = intrwait;
  intrwait = p;
	
  unmaskintr(irqn);
	
  return true;
}

static bool
irqhandler(void)
{
  struct proc *p, *pp;
  uint32_t irq;
  bool r;
	
  r = false;
  irq = readl(INTC + INTC_SIR_IRQ);

  debug("irq: %i\n", irq);

  if (handlers[irq]) {
    /* Kernel handler */
    r = handlers[irq](irq);
  } else {
    /* User proc handler */
    pp = nil;
    for (p = intrwait; p != nil; pp = p, p = p->wnext) {
      if (p->waiting.intr == irq) {
	if (pp != nil) {
	  pp->wnext = p->wnext;
	} else {
	  intrwait = p->wnext;
	}
	
	procready(p);
	maskintr(irq);
	r = true;
	break;
      }
    }
  }
	
  /* Allow new interrupts */
  writel(1, INTC + INTC_CONTROL);
	
  return r;
}

void
trap(struct ureg *ureg)
{
  uint32_t fsr;
  void *addr;
  bool fixed = true;
  bool rsch = false;

  current->inkernel = true;

  if (ureg->type == ABORT_DATA)
    ureg->pc -= 8;
  else
    ureg->pc -= 4;

  switch(ureg->type) {
  case ABORT_INTERRUPT:
    rsch = irqhandler();
    break;
  case ABORT_INSTRUCTION:
    fixed = false;
    printf("%i bad instruction at 0x%h\n",
	   current->pid, ureg->pc);
    break;
  case ABORT_PREFETCH:
    fixed = fixfault((void *) ureg->pc);
    if (!fixed) {
      printf("%i prefetch abort 0x%h\n",
	     current->pid, ureg->pc);
    }

    break;
  case ABORT_DATA:
    addr = faultaddr();
    fsr = fsrstatus() & 0xf;
    fixed = false;

    switch (fsr) {
    case 0x0: /* vector */
    case 0x1: /* alignment */
    case 0x3: /* also alignment */
    case 0x2: /* terminal */
    case 0x4: /* external linefetch section */
    case 0x6: /* external linefetch page */
      break;
    case 0x5: /* section translation */
    case 0x7: /* page translation */
      /* Try add page */
      fixed = fixfault(addr);
      break;
    case 0x8: /* external non linefetch section */
    case 0xa: /* external non linefetch page */
    case 0x9: /* domain section */
    case 0xb: /* domain page */
    case 0xc: /* external translation l1 */
    case 0xe: /* external translation l2 */
    case 0xd: /* section permission */
    case 0xf: /* page permission */
      break;
    }

    if (!fixed) {
      printf("%i data abort 0x%h (type 0x%h)\n",
	     current->pid, addr, fsr);
    }
		
    break;
  }
	
  if (!fixed) {
    printf("kill proc %i for doing something bad\n",
	   current->pid);
    dumpregs(ureg);
    procremove(current);
    schedule();
  }
	
  if (rsch)
    schedule();

  current->inkernel = false;
}
