#include <stdarg.h>
#include <types.h>
#include <io.h>
#include <com.h>

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
	if (den == 10) {
		i = *num / 10;
		*num = *num % 10;
		return i;
	} else if (den == 16) {
		i = *num / 16;
		*num = *num % 16;
		return i;
	} else {
		while (*num >= den) {
			*num -= den;
			i++;
		}
	}
	return i;
}

static void
print_int(unsigned int i, unsigned int base)
{
	unsigned int d;
	unsigned char str[8];
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
