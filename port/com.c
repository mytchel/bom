#include "types.h"
#include "../include/com.h"
#include "../include/stdarg.h"

void
puts(char *str) {
	while (*str) {
		putc(*str++);
	}
}

static void
print_int(unsigned int i, unsigned int base)
{
	unsigned int d;
	unsigned char str[48];
	int c = 0;
	
	do {
		d = i / base;
		i = i % base;
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
