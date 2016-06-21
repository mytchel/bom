#include "dat.h"
#include "../port/com.h"
#include "../port/stdarg.h"

void
puts(const char *str) {
	while (*str) {
		putc(*str++);
	}
}

static void
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

	while (c > 0 && max > 0) {
		*str = s[--c];
		max--;
		str++;
	}
}

void 
kprintf(const char *fmt, ...)
{
	int i;
	unsigned int ind, u;
	char *s, str[MAX_STR_LEN];
	va_list ap;
	
	va_start(ap, fmt);
	ind = 0;
	while (*fmt != 0 && ind < MAX_STR_LEN) {
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

			if (ind == MAX_STR_LEN)
				break;

			print_int(str + ind, MAX_STR_LEN - ind, (unsigned int) i, 10);		
			break;
		case 'u':
			u = va_arg(ap, unsigned int);
			print_int(str + ind, MAX_STR_LEN - ind, u, 10);
			break;
		case 'h':
			u = va_arg(ap, unsigned int);
			print_int(str + ind, MAX_STR_LEN - ind, u, 16);
			break;
		case 'b':
			u = va_arg(ap, unsigned int);
			print_int(str + ind, MAX_STR_LEN - ind, u, 2);
			break;
		case 'c':
			i = va_arg(ap, int);
			str[ind++] = i;
			break;
		case 's':
			s = va_arg(ap, char*);
			for (i = 0; ind < MAX_STR_LEN && s[i]; i++)
				str[ind++] = s[i];
			break;
		}
		
		fmt++;
	}
	va_end(ap);
	
	puts(str);
}
