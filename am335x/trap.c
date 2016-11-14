/*
 *
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

#include "../kern/head.h"
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
maskintr(uint32_t irqn);
static void
unmaskintr(uint32_t irqn);

static void (*handlers[nirq])(uint32_t) = {0};
static struct proc *waiting[nirq] = {nil};

void
intcinit(void)
{
  int i;

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
maskintr(uint32_t irqn)
{
  uint32_t mask, mfield;

  mfield = irqn / 32;
  mask = 1 << (irqn % 32);

  writel(mask, INTC + INTC_SETn(mfield));
}

void
unmaskintr(uint32_t irqn)
{
  uint32_t mask, mfield;

  mfield = irqn / 32;
  mask = 1 << (irqn % 32);

  writel(mask, INTC + INTC_CLEARn(mfield));
}

void
intcaddhandler(uint32_t irqn, void (*func)(uint32_t))
{
  handlers[irqn] = func;
  unmaskintr(irqn);
}

bool
procwaitintr(int irqn)
{
  intrstate_t t;
  
  if (irqn < 0 || irqn > nirq) {
    return false;
  } else if (handlers[irqn] != nil) {
    return false;
  }

  if (!cas(&waiting[irqn], nil, up)) {
    return false;
  }

  procwait(up);

  t = setintr(INTR_OFF);
  unmaskintr(irqn);
  schedule();
  setintr(t);

  return true;
}

static void
irqhandler(void)
{
  struct proc *p;
  uint32_t irq;
	
  irq = readl(INTC + INTC_SIR_IRQ);

  /* Allow new interrupts */
  writel(1, INTC + INTC_CONTROL);

  if (handlers[irq]) {
    /* Kernel handler */
    handlers[irq](irq);
  } else {
    /* User proc handler */
    p = waiting[irq];
    if (p == nil) {
      return;
    }

    waiting[irq] = nil;
    maskintr(irq);

    procready(p);
    schedule();
  }
}

void
trap(void *pc, int type)
{
  uint32_t fsr;
  void *addr;

  if (up == nil) {
    printf("Trapped with an unknown process!\n");
    panic("Probably an in kernel problem.\n");
  }

  switch(type) {
  case ABORT_INTERRUPT:
    irqhandler();

    return; /* Note the return. */

  case ABORT_INSTRUCTION:
    printf("%i bad instruction at 0x%h\n",
	   up->pid, pc);
    break;

  case ABORT_PREFETCH:
    if (fixfault((void *) pc)) {
      return;
    }

    printf("%i prefetch abort 0x%h\n",
	   up->pid, pc);

    break;

  case ABORT_DATA:
    addr = faultaddr();
    fsr = fsrstatus() & 0xf;

    switch (fsr) {
    case 0x5: /* section translation */
    case 0x7: /* page translation */
      /* Try add page */
      if (fixfault(addr)) {
	return;
      }
      
      break;
    case 0x0: /* vector */
    case 0x1: /* alignment */
    case 0x3: /* also alignment */
    case 0x2: /* terminal */
    case 0x4: /* external linefetch section */
    case 0x6: /* external linefetch page */
    case 0x8: /* external non linefetch section */
    case 0xa: /* external non linefetch page */
    case 0x9: /* domain section */
    case 0xb: /* domain page */
    case 0xc: /* external translation l1 */
    case 0xe: /* external translation l2 */
    case 0xd: /* section permission */
    case 0xf: /* page permission */
    default:
      break;
    }

    printf("%i data abort 0x%h (type 0x%h)\n",
	   up->pid, addr, fsr);
		
    break;
  }
	
  printf("kill proc %i (trap %i)\n", up->pid, type);
  procexit(up, -1);
  schedule();
  /* Never reached */
}
