/*
 *   Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <types.h>
#include <stdarg.h>
#include <string.h>

static size_t
printint(char *str, size_t max, unsigned int i, unsigned int base)
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

size_t
vsnprintf(char *str, size_t max, const char *fmt, va_list ap)
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

			if (ind >= max)
				break;

			ind += printint(str + ind, max - ind,
				(unsigned int) i, 10);		
			break;
		case 'u':
			u = va_arg(ap, unsigned int);
			ind += printint(str + ind, max - ind,
				 u, 10);
			break;
		case 'h':
			u = va_arg(ap, unsigned int);
			ind += printint(str + ind, max - ind, 
				u, 16);
			break;
		case 'b':
			u = va_arg(ap, unsigned int);
			ind += printint(str + ind, max - ind, 
				u, 2);
			break;
		case 'c':
			i = va_arg(ap, int);
			str[ind++] = i;
			break;
		case 's':
			s = va_arg(ap, char *);
			if (s == nil) {
				s = "(null)";
			}
			
			for (i = 0; ind < max && s[i]; i++)
				str[ind++] = s[i];
			break;
		}
		
		fmt++;
	}

	str[ind] = 0;
	return ind;
}

size_t
snprintf(char *str, size_t max, const char *fmt, ...)
{
	int i;
	va_list ap;
	
	va_start(ap, fmt);
	i = vsnprintf(str, max, fmt, ap);
	va_end(ap);
	return i;
}
