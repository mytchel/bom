#include "dat.h"

#define UART0   	0x44E09000
#define UART0_LEN   	0x1000

struct uart_struct {
	uint32_t hr;
	uint32_t p1;
	uint32_t p2;
	uint32_t lcr;
	uint32_t p4;
	uint32_t lsr;
};

static struct uart_struct *uart;

bool
uartinit(void)
{
	size_t size = UART0_LEN;
	
	uart = (struct uart_struct *) getmem(MEM_io, (void *) UART0, &size);
	if (uart == nil)
		return false;
	
	puts("User UART Initialised\n");
	
	return true;
}

char
getc(void)
{
	while ((uart->lsr & (1 << 0)) == 0)
		sleep(0);
	return (char) (uart->hr & 0xff);
}

void
putc(char c)
{
	if (c == '\n')
		putc('\r');
	
	while ((uart->lsr & (1 << 5)) == 0)
		sleep(0);
	uart->hr = c;
}

void
puts(const char *c)
{
	while (*c)
		putc(*c++);
}
