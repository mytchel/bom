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
#define TIMER_TWPS		0x48

#define CM_DPLL	0x44E00500
#define CLK_SEL2 0x08

#define WDT_1 0x44E35000

#define WDT_WSPR	0x48
#define WDT_WWPS	0x34

static void systickhandler(uint32_t);
static uint32_t mstoticks(uint32_t);

/* In KHz */
static uint32_t freq;

void
initwatchdog(void)
{
	/* Disable watchdog timer. */
	
	writel(0x0000AAAA, WDT_1 + WDT_WSPR);
	while (readl(WDT_1 + WDT_WWPS) & (1<<4));
	writel(0x00005555, WDT_1 + WDT_WSPR);
	while (readl(WDT_1 + WDT_WWPS) & (1<<4));
}

void inittimers()
{
	/* Select 32KHz clock for timers */
 	writel(2, CM_DPLL + CLK_SEL2);
 	freq = 32;
	
	writel(0, TIMER2 + TIMER_TCLR); /* disable timer */
	writel(1, TIMER2 + TIMER_IRQENABLE_SET); /* set irq */

 	while (readl(TIMER2 + TIMER_TWPS)); /* Wait for writes to commit. */

	intcaddhandler(TINT2, &systickhandler);
}

void
systickhandler(uint32_t irqn)
{
	writel(0, TIMER2 + TIMER_TCLR); /* disable timer */
	/* Clear irq status. */
	writel(readl(TIMER2 + TIMER_IRQSTATUS), 
		TIMER2 + TIMER_IRQSTATUS);
 	while (readl(TIMER2 + TIMER_TWPS)); /* Wait for writes to commit. */
}

void
setsystick(uint32_t ms)
{
	writel(0, TIMER2 + TIMER_TCRR); /* set timer to 0 */
	writel(mstoticks(ms), TIMER2 + TIMER_TMAR); /* set compare value */
 	
 	while (readl(TIMER2 + TIMER_TWPS)); /* Wait for writes to commit. */
	
	writel((1<<6) |1, TIMER2 + TIMER_TCLR); /* start timer */
}

uint32_t
mstoticks(uint32_t ms)
{
	return ms * freq;
}
