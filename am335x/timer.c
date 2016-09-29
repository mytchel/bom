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

#include "head.h"
#include "fns.h"

#define TIMER0 0x44E05000
#define TIMER2 0x48040000

#define TINT0 66
#define TINT2 68

#define TIMER_IRQSTATUS		0x28
#define TIMER_IRQENABLE_SET	0x2c
#define TIMER_IRQENABlE_CLR	0x30
#define TIMER_TCLR		0x38
#define TIMER_TCRR		0x3c
#define TIMER_TMAR		0x4c
#define TIMER_TWPS		0x48

#define CM_DPLL	0x44E00500
#define CLK_SEL0 0x08
#define CLK_SEL1 0x08
#define CLK_SEL2 0x08

#define WDT_1 0x44E35000

#define WDT_WSPR	0x48
#define WDT_WWPS	0x34

static bool systickhandler(uint32_t);

void
initwatchdog(void)
{
  /* Disable watchdog timer. */
  writel(0x0000AAAA, WDT_1 + WDT_WSPR);
  while (readl(WDT_1 + WDT_WWPS) & (1<<4));
  writel(0x00005555, WDT_1 + WDT_WSPR);
  while (readl(WDT_1 + WDT_WWPS) & (1<<4));
}

void
inittimers(void)
{
  /* Select 32KHz clock for timer 2 */
  writel(2, CM_DPLL + CLK_SEL2);

  writel(0, TIMER2 + TIMER_TCLR); /* disable timer */
  writel(1, TIMER2 + TIMER_IRQENABLE_SET); /* set irq */
  /* Wait for writes to commit. */
  while (readl(TIMER2 + TIMER_TWPS)); 

  intcaddhandler(TINT2, &systickhandler);

  /* Set up tick timer on timer 0. */

  writel(0, TIMER0 + TIMER_TCRR); /* set timer to 0 */
  writel(1, TIMER0 + TIMER_TCLR); /* start timer */
  /* Wait for writes to commit. */
  while (readl(TIMER0 + TIMER_TWPS));
}

bool
systickhandler(uint32_t irqn)
{
  /* Clear irq status. */
  writel(readl(TIMER2 + TIMER_IRQSTATUS), 
	 TIMER2 + TIMER_IRQSTATUS);

  writel(0, TIMER2 + TIMER_TCLR); /* disable timer */

  /* Wait for writes to commit. */
  while (readl(TIMER2 + TIMER_TWPS)); 
	
  return true;
}

void
setsystick(uint32_t ms)
{
  uint32_t t = mstoticks(ms);
	
  writel(0, TIMER2 + TIMER_TCLR); /* disable timer */
  /* Wait for writes to commit. */
  while (readl(TIMER2 + TIMER_TWPS)); 
	
  writel(0, TIMER2 + TIMER_TCRR); /* set timer to 0 */
  writel(t, TIMER2 + TIMER_TMAR); /* set compare value */

  /* Clear irq status if it is set. */
  writel(readl(TIMER2 + TIMER_IRQSTATUS), 
	 TIMER2 + TIMER_IRQSTATUS);

  /* Wait for writes to commit. */	
  while (readl(TIMER2 + TIMER_TWPS)); 
	
  writel((1<<6) | 1, TIMER2 + TIMER_TCLR); /* start timer */
}

uint32_t
ticks(void)
{
  uint32_t t;

  t = readl(TIMER0 + TIMER_TCRR);
  writel(0, TIMER0 + TIMER_TCRR); /* reset timer */

  return t;
}

uint32_t
mstoticks(uint32_t ms)
{
  return ms * 32;
}

uint32_t
tickstoms(uint32_t t)
{
  return t / 32;
}

