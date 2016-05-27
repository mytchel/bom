#include <types.h>
#include <com.h>
#include <io.h>

int main(void)
{
	char c;
	uint32_t *addr = (uint32_t *) 328473222;

	while (1) {
		uart_puts("waiting: ");
		c = uart_getc();	
		uart_puts("got: ");
		uart_putc(c);
		uart_putc('\n');
		
		if (c == 'q') {
			uart_puts("going to access some fucked up address\n");
			char a = readl(addr);
			uart_putc(a);
			uart_putc('\n');
			addr += 97271132;
		}
	}	
}
