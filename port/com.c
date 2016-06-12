#include "types.h"
#include "../include/com.h"
#include "../include/stdarg.h"

void
puts(char *str) {
	while (*str) {
		putc(*str++);
	}
}

static unsigned int
div(unsigned int *num, unsigned int den)
{
	unsigned int i = 0;
	unsigned int n = *num;
	
	if (den == 10) {
		i = n / 10;
		n = n % 10;
	} else if (den == 16) {
		i = n / 16;
		n = n % 16;
	} else if (den == 2) {
		i = n / 2;
		n = n % 2;
	} else {
		while (n >= den) {
			n -= den;
			i++;
		}
	}

	*num = n;
	return i;
}

static void
print_int(unsigned int i, unsigned int base)
{
	unsigned int d;
	unsigned char str[48];
	int c = 0;
	
	do {
		d = div(&i, base);
		if (i > 9) str[c++] = 'a' + (i-10);
		else str[c++] = '0' + i;
		i = d;
	} while (i > 0);

	while (c > 0) {
		putc(str[--c]);
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
			putc(*str++);
			continue;
		}
		
		str++;
		switch (*str) {
		case '%':
			putc('%');
			break;
		case 'i':
			i = va_arg(ap, int);
			if (i < 0) {
				putc('-');
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
		case 'b':
			u = va_arg(ap, unsigned int);
			print_int(u, 2);
			break;
		case 'c':
			i = va_arg(ap, int);
			putc(i);
			break;
		case 's':
			s = va_arg(ap, char*);
			puts(s);
			break;
		}
		str++;
	}
	va_end(ap);
}
