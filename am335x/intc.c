#include "types.h"
#include "io.h"
#include "intc.h"
#include "../include/com.h"

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

static uint32_t intc_nirq = 128;

static void (*handlers[128])(uint32_t);

void
intc_init(void)
{
	int i;
	
	kprintf("intc_init\n");

	/* enable interface auto idle */
	writel(1, INTC + INTC_SYSCONFIG);

	kprintf("mask interrupts\n");
	/* mask all interrupts. */
	for (i = 0; i < intc_nirq / 32; i++) {
		writel(0xffffffff, INTC + INTC_MIRn(i));
	}
	
	/* Set all interrupts to lowest priority. */
	for (i = 0; i < intc_nirq; i++) {
		writel(63 << 2, INTC + INTC_ILRn(i));
	}
}

void
intc_irq_handler(void)
{
	uint32_t irq = readl(INTC + INTC_SIR_IRQ);
	
	if (handlers[irq]) {
		handlers[irq](irq);
	}
	
	/* Allow new interrupts */
	writel(1, INTC + INTC_CONTROL);
}

void
intc_add_handler(uint32_t irqn, void (*func)(uint32_t))
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