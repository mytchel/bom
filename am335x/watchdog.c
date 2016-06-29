#include "dat.h"
#include "fns.h"
#include "../port/com.h"

#define WDT_1 0x44E35000

#define WDT_WSPR	0x48
#define WDT_WWPS	0x34

void
watchdoginit(void)
{
	/* Disable watchdog timer for now. */
	
	kprintf("Disabling watchdog timer\n");
	
	writel(0x0000AAAA, WDT_1 + WDT_WSPR);
	while (readl(WDT_1 + WDT_WWPS) & (1<<4));
	writel(0x00005555, WDT_1 + WDT_WSPR);
	while (readl(WDT_1 + WDT_WWPS) & (1<<4));
}
