#include <stdarg.h>
#include <types.h>
#include <io.h>
#include <com.h>

#define UART0   0x44E09000

struct uart {
	union { 		/* 0x00 */
		uint32_t rbr;
		uint32_t thr;
	};
	uint32_t ier; 		/* 0x04 */
	union {			/* 0x08 */
		uint32_t iir;
		uint32_t fcr;
	};
	uint32_t lcr;		/* 0x0c */
	uint32_t mcr;		/* 0x10 */
	uint32_t lsr;		/* 0x14 */
	uint32_t msr;		/* 0x18 */
	uint32_t scr;		/* 0x1c */
	uint32_t dll;		/* 0x20 */
	uint32_t dlh;		/* 0x24 */
	uint32_t revid1;	/* 0x28 */
	uint32_t revid2;	/* 0x2c */
	uint32_t pwremu_mgmt;	/* 0x30 */
	uint32_t mdr;		/* 0x34 */
};

static struct uart *uart = (struct uart *) UART0;

static char
uart_getc()
{
	while ((readl(&uart->lsr) & (1 << 0)) == 0);
	return (char) readl(&uart->rbr);
}

static void
uart_putc(char c)
{
	if (c == '\n')
		uart_putc('\r');
	
	while ((readl(&uart->lsr) & (1 << 5)) == 0);
	writel(c, &uart->thr);
}

void putc(char c) {
	uart_putc(c);
}

char getc() {
	return uart_getc();
}

