#include <io.h>
#include <com.h>
#include <types.h>

void
abort_handler(uint32_t ptr, uint32_t code)
{
	uart_puts("abort hander: ");
	if (code) {
		uart_puts("data\n");
	} else {
		uart_puts("prefetch\n");
	}
}
