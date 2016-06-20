#include "io.h"
#include "../port/types.h"
#include "../port/com.h"

#define UART0   0x44E09000

#define UART_LSR	0x14
#define UART_RBR	0x00
#define UART_THR	0x00

char
getc(void)
{
	while ((readl(UART0 + UART_LSR) & (1 << 0)) == 0);
	return (char) readl(UART0 + UART_RBR);
}

void
putc(char c)
{
	if (c == '\n')
		putc('\r');
	
	while ((readl(UART0 + UART_LSR) & (1 << 5)) == 0);
	writel(c, UART0 + UART_THR);
}
