#ifndef __IO_H
#define __IO_H

#define readl(a)	(*(volatile uint32_t*)(a))
#define readw(a)	(*(volatile uint16_t*)(a))
#define readb(a)	(*(volatile uint8_t*)(a))

#define writel(v, a)	(*(volatile uint32_t*)(a) = (uint32_t)(v))
#define writew(v, a)	(*(volatile uint16_t*)(a) = (uint16_t)(v))
#define writeb(v, a)	(*(volatile uint8_t*)(a) = (uint8_t)(v))

#endif
