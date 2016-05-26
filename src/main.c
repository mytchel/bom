#include <types.h>
#include <kprint.h>
#include <com.h>

int main(void)
{
	char c;

	kprintf("Kernel Starting...\n");

	while (1) {
		c = uart_getc();	
		kprintf("got: ");
		uart_putc(c);
		uart_putc('\n');
	}	
}
