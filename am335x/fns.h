#ifndef __FNS_H
#define __FNS_H

#define readl(a)	(*(volatile uint32_t*)(a))
#define readw(a)	(*(volatile uint16_t*)(a))
#define readb(a)	(*(volatile uint8_t*)(a))

#define writel(v, a)	(*(volatile uint32_t*)(a) = (uint32_t)(v))
#define writew(v, a)	(*(volatile uint16_t*)(a) = (uint16_t)(v))
#define writeb(v, a)	(*(volatile uint8_t*)(a) = (uint8_t)(v))

#define L1_FAULT	0
#define L1_COARSE	1
#define L1_SECTION	2
#define L1_FINE		3

#define L2_FAULT	0
#define L2_LARGE	1
#define L2_SMALL	2
#define L2_TINY		3

void
dumpregs(struct ureg *);

void
uregret(struct ureg *);

void
droptouser(void *);

uint32_t
fsrstatus(void);

void *
faultaddr(void);

void
intcinit(void);

void
intcaddhandler(uint32_t, void (*)(uint32_t));

void
mmuinvalidate(void);

void
imap(uint32_t, uint32_t);
	
void
memoryinit(void);

void
mmuempty1(void);

int
pageinit(void *, size_t);

int
heapinit(void *, size_t);

void
mmuloadttb(uint32_t *);

void
mmuinit(void);

#endif
