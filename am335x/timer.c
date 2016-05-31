#include <io.h>
#include <com.h>

#define TIMER0 0x44E05000
#define TIMER1 0x44E31000
#define TIMER2 0x48040000
#define TIMER3 0x48042000
#define TIMER4 0x48044000
#define TIMER5 0x48046000
#define TIMER6 0x48048000
#define TIMER7 0x4804A000

struct timer {
	uint32_t tidr;
	uint32_t pad1[3];
	uint32_t tiocp_cfg;
	uint32_t pad2[4];
	uint32_t irq_eoi;
	uint32_t irq_status_raw;
	uint32_t irq_status;
	uint32_t irq_enable_set;
	uint32_t irq_enable_clr;
	uint32_t irq_wakeen;
	uint32_t tclr;
	uint32_t tcrr;
	uint32_t tldr;
	uint32_t ttgr;
	uint32_t twps;
	uint32_t tmar;
	uint32_t tcar1;
	uint32_t tsicr;
	uint32_t tcar2;
};

struct timer_1ms {
	uint32_t tidr;
	uint32_t pad1[3];
	uint32_t tiocp_cfg;
	uint32_t tistat;
	uint32_t tisr;
	uint32_t tier;
	uint32_t twer;
	uint32_t tclr;
	uint32_t tcrr;
	uint32_t tldr;
	uint32_t ttgr;
	uint32_t twps;
	uint32_t tmar;
	uint32_t tcar1;
	uint32_t tsicr;
	uint32_t tcar2;
	uint32_t tpir;
	uint32_t tnir;
	uint32_t tcvr;
	uint32_t tocr;
	uint32_t towr;
};


//static struct timer_1ms *timer = (struct timer_1ms *) TIMER1;

void timerInit()
{
	writel(0, TIMER2 + 0x38); /* disable timer */
	writel(0, TIMER2 + 0x3c); /* set timer to 0 */
	writel(1, TIMER2 + 0x2c); /* set irq */
	
	kprintf("irqstatus: %h\n", readl(TIMER2 + 0x28));
	writel(1000, TIMER2 + 0x4c); /* set compare value */
	writel((1 << 6) | 1, TIMER2 + 0x38); /* set compare and enable timer */
	
	kprintf("irqstatus: %h\n", readl(TIMER2 + 0x28));
	kprintf("compare : %i\n", readl(TIMER2 + 0x4c));
	kprintf("tclr: %h\n", readl(TIMER2 + 0x38));

	while (1) {
		kprintf("irqstatus: %h\n", readl(TIMER2 + 0x28));
		kprintf("timer: %i\n", readl(TIMER2 + 0x3c));
		if (readl(TIMER2 + 0x3c) > 100000)
			asm("ldr pc, =#0x32324843");
	}
	
	/*
	kprintf("init timers\n");
	writel(10, TIMER2 + 0x2c);
	writel(1, &timer->tier);
	writel((1<<6) & 1, &timer->tclr);
	
	kprintf("should have set, so checking\n");
	tier = *((uint32_t *) TIMER1 + 0x1c);//readl(&timer->tier);
	kprintf("tier: 0x%h\n", tier);
	*/
}
