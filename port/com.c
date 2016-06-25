#include "dat.h"
#include "../port/com.h"
#include "../include/stdarg.h"

static int
print_int(char *str, size_t max, unsigned int i, unsigned int base)
{
	unsigned int d;
	unsigned char s[32];
	int c = 0;
	
	do {
		d = i / base;
		i = i % base;
		if (i > 9) s[c++] = 'a' + (i-10);
		else s[c++] = '0' + i;
		i = d;
	} while (i > 0);

	d = 0;
	while (c > 0 && d < max) {
		str[d++] = s[--c];
	}
	
	return d;
}

int
vsprintf(char *str, int max, const char *fmt, va_list ap)
{
	int i;
	unsigned int ind, u;
	char *s;
	
	ind = 0;
	while (*fmt != 0 && ind < max) {
		if (*fmt != '%') {
			str[ind++] = *fmt++;
			continue;
		}
		
		fmt++;
		switch (*fmt) {
		case '%':
			str[ind++] = '%';
			break;
		case 'i':
			i = va_arg(ap, int);
			if (i < 0) {
				str[ind++] = '-';
				i = -i;
			}

			if (ind == max)
				break;

			ind += print_int(str + ind, max - ind,
				(unsigned int) i, 10);		
			break;
		case 'u':
			u = va_arg(ap, unsigned int);
			ind += print_int(str + ind, max - ind,
				 u, 10);
			break;
		case 'h':
			u = va_arg(ap, unsigned int);
			ind += print_int(str + ind, max - ind, 
				u, 16);
			break;
		case 'b':
			u = va_arg(ap, unsigned int);
			ind += print_int(str + ind, max - ind, 
				u, 2);
			break;
		case 'c':
			i = va_arg(ap, int);
			str[ind++] = i;
			break;
		case 's':
			s = va_arg(ap, char*);
			for (i = 0; ind < max && s[i]; i++)
				str[ind++] = s[i];
			break;
		}
		
		fmt++;
	}

	str[ind] = 0;
	return ind;
}

int
sprintf(char *str, int max, const char *fmt, ...)
{
	int i;
	va_list ap;
	
	va_start(ap, fmt);
	i = vsprintf(str, max, fmt, ap);
	va_end(ap);
	return i;
}

void
kprintf(const char *fmt, ...)
{
	char str[512];
	va_list ap;
	
	va_start(ap, fmt);
	vsprintf(str, 512, fmt, ap);
	va_end(ap);
	
	/* For now */
	puts(str);
}
