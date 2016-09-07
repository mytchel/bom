#ifndef __FNS_H
#define __FNS_H

#include "trap.h"

#define readl(a)	(*(volatile uint32_t*)(a))
#define readw(a)	(*(volatile uint16_t*)(a))
#define readb(a)	(*(volatile uint8_t*)(a))

#define writel(v, a)	(*(volatile uint32_t*)(a) = (uint32_t)(v))
#define writew(v, a)	(*(volatile uint16_t*)(a) = (uint16_t)(v))
#define writeb(v, a)	(*(volatile uint8_t*)(a) = (uint8_t)(v))

#define AP_RW_NO	1
#define AP_RW_RO	2
#define AP_RW_RW	3

#define PAGE_SHIFT 	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))
#define PAGE_ALIGN(x) 	(((x) + PAGE_SIZE - 1) & PAGE_MASK)

void
uregret(struct ureg *);

void
droptouser(void *);

void *
forkfunc_preloader(void *, void *, void *);

void
forkfunc_loader(void);

uint32_t
fsrstatus(void);

void *
faultaddr(void);

void
intcaddhandler(uint32_t, bool (*)(uint32_t));

void
mmuinvalidate(void);

void
imap(void *, void *, int);
	
void
mmuempty1(void);

void
mmuloadttb(uint32_t *);

/* Initialisation functions */

void
initintc(void);

void
initmemory(void);

void
initmmu(void);

void
initwatchdog(void);

void
inittimers(void);

#endif
