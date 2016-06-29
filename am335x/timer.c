#include "dat.h"
#include "fns.h"
#include "../port/com.h"

#define TIMER0 0x44E05000
#define TIMER1 0x44E31000
#define TIMER2 0x48040000
#define TIMER3 0x48042000
#define TIMER4 0x48044000
#define TIMER5 0x48046000
#define TIMER6 0x48048000
#define TIMER7 0x4804A000

#define TINT0 66
#define TINT1 67
#define TINT2 68
#define TINT3 69
#define TINT4 92
#define TINT5 93
#define TINT6 94
#define TINT7 95

#define TIMER_IRQSTATUS		0x28
#define TIMER_IRQENABLE_SET	0x2c
#define TIMER_IRQENABlE_CLR	0x30
#define TIMER_TCLR		0x38
#define TIMER_TCRR		0x3c
#define TIMER_TMAR		0x4c

static void systickhandler(uint32_t);

void systickinit(void)
{
	writel(0, TIMER2 + TIMER_TCLR); /* disable timer */
	writel(0, TIMER2 + TIMER_TCRR); /* set timer to 0 */
	writel(1, TIMER2 + TIMER_IRQENABLE_SET); /* set irq */
	
	writel(1000000, TIMER2 + TIMER_TMAR); /* set compare value */

	intcaddhandler(TINT2, &systickhandler);

	/* Start timer */
	writel((1<<6) |1, TIMER2 + TIMER_TCLR);
}

void
systickhandler(uint32_t irqn)
{
	writel(0, TIMER2 + TIMER_TCRR); /* set timer to 0 */
	/* Clear irq status. */
	writel(readl(TIMER2 + TIMER_IRQSTATUS), 
		TIMER2 + TIMER_IRQSTATUS);
}
