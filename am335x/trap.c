#include "dat.h"
#include "trap.h"
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
#define INTC_SET(i)		0x80+(0x20*i)+0x0c
#define INTC_ISR_SETn(i)	0x80+(0x20*i)+0x10
#define INTC_ISR_CLEARn(i)	0x80+(0x20*i)+0x14
#define INTC_PENDING_IRQn(i)	0x80+(0x20*i)+0x18
#define INTC_PENDING_FIQn(i)	0x80+(0x20*i)+0x1c

#define INTC_ILRn(i)		0x100+(0x04*i)

#define nirq 128

static void (*handlers[nirq])(uint32_t);

void
initintc(void)
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
}

static void
irqhandler(void)
{
	uint32_t irq = readl(INTC + INTC_SIR_IRQ);
		
	if (handlers[irq]) {
		handlers[irq](irq);
	}
	
	/* Allow new interrupts */
	writel(1, INTC + INTC_CONTROL);
}

void
intcaddhandler(uint32_t irqn, void (*func)(uint32_t))
{
	uint32_t mask, mfield;
	
	handlers[irqn] = func;
	
	/* Unmask interrupt number. */
	mfield = irqn / 32;
	mask = 1 << (irqn % 32);
	writel(mask, INTC + INTC_CLEARn(mfield));

	writel(1, INTC + INTC_CONTROL);
	
	kprintf("intc handler set for %i\n", irqn);
}

void
trap(struct ureg *ureg)
{
	uint32_t fsr;
	void *addr;
	bool fixed = false;

	if (ureg->type == ABORT_DATA)
		ureg->pc -= 8;
	else
		ureg->pc -= 4;
	
	switch(ureg->type) {
	case ABORT_INTERRUPT:
		fixed = true;
		irqhandler();
		break;
	case ABORT_INSTRUCTION:
		kprintf("undefined instruction\n");
		break;
	case ABORT_PREFETCH:
		kprintf("prefetch abort: 0x%h\n", ureg->pc);
		fixed = fixfault((void *) ureg->pc);
		break;
	case ABORT_DATA:
		addr = faultaddr();
		fsr = fsrstatus() & 0xf;
		
		kprintf("data abort: 0x%h accessing 0x%h\n", fsr, addr);
		
		switch (fsr) {
		case 0x0: /* vector */
			break;
		case 0x1: /* alignment */
		case 0x3: /* also alignment */
			break;
		case 0x2: /* terminal */
			break;
		case 0x4: /* external linefetch section */
			break;
		case 0x6: /* external linefetch page */
			break;
		case 0x5: /* section translation */
		case 0x7: /* page translation */
			/* Try add page */
			fixed = fixfault(addr);
			break;
		case 0x8: /* external non linefetch section */
			break;
		case 0xa: /* external non linefetch page */
			break;
		case 0x9: /* domain section */
		case 0xb: /* domain page */
			break;
		case 0xc: /* external translation l1 */
		case 0xe: /* external translation l2 */
			break;
		case 0xd: /* section permission */
		case 0xf: /* page permission */
			kprintf("may need to change page permissions.\n"
				"for now just kill\n");
			break;
		}
		
		break;
	}
	
	if (!fixed) {
		kprintf("kill proc for doing something bad\n");
		dumpregs(ureg);
		procremove(current);
	}
	
	schedule();
}
