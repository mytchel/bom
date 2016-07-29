#include "dat.h"

#define UART0   	0x44E09000
#define UART0_LEN   	0x1000

struct uart_struct {
	uint32_t hr;
	uint32_t pad[4];
	uint32_t lsr;
};

static struct uart_struct *uart;

void uputc(char);

bool
uartinit(void)
{
	size_t size = UART0_LEN;
	
	uart = (struct uart_struct *) getmem((void *) UART0, &size);
	if (uart == nil)
		return false;
	
	printf("Uart Initialised 0x%h\n", uart);
	sleep(0);
	uputc('\n');
	uputc('\n');
	uputc('\n');
	printf("should have printed some newlines\n");
	
	return true;
}


char
ugetc(void)
{
	while ((uart->lsr & (1 << 0)) == 0);
	return (char) uart->hr;
}

void
uputc(char c)
{
	if (c == '\n')
		uputc('\r');
	
	while ((uart->lsr & (1 << 5)) == 0);
	uart->hr = c;
}

void
uputs(const char *c)
{
	while (*c)
		uputc(*c++);
}
