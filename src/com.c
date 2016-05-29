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

void
uart_putc(char c)
{
	if (c == '\n')
		uart_putc('\r');
	
	while ((readl(&uart->lsr) & (1 << 5)) == 0);
	writel(c, &uart->thr);
}

void
uart_puts(const char *str)
{
	while (*str) {
		uart_putc(*str++);
	}
}

static unsigned int
div(unsigned int *num, unsigned int den)
{
	unsigned int i = 0;
	while (*num > den) {
		*num -= den;
		i++;
	}
	return i;
}

static void
print_int(unsigned int i, unsigned int base)
{
	unsigned int d;
	unsigned char str[8];
	int c = 0;
	
	while (i > 0) {
		d = div(&i, base);
		
		if (i > 9) str[c++] = 'a' + (i-10);
		else str[c++] = '0' + i;
		
		i = d;
	}

	while (c > 0) {
		uart_putc(str[--c]);
	}
}

void 
kprintf(const char *str, ...)
{
	int i;
	unsigned int u;
	char *s;
	
	va_list ap;
	
	va_start(ap, str);
	while (*str != 0) {
		if (*str != '%') {
			uart_putc(*str++);
			continue;
		}
		
		str++;
		switch (*str) {
		case '%':
			uart_putc('%');
			break;
		case 'i':
			i = va_arg(ap, int);
			if (i < 0) {
				uart_putc('-');
				i = -i;
			}

			print_int((unsigned int) i, 10);		
			break;
		case 'u':
			u = va_arg(ap, unsigned int);
			print_int(u, 10);
			break;
		case 'h':
			u = va_arg(ap, unsigned int);
			print_int(u, 16);
			break;
		case 'c':
			i = va_arg(ap, int);
			uart_putc(i);
			break;
		case 's':
			s = va_arg(ap, char*);
			uart_puts(s);
			break;
		}
		str++;
	}
	va_end(ap);
}

char
uart_getc()
{
	while ((readl(&uart->lsr) & (1 << 0)) == 0);
	return (char) readl(&uart->rbr);
}

