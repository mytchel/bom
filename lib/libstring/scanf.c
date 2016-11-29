/*
 *
 * Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <libc.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

int
vfscanf(int fd, const char *fmt, va_list ap)
{
  return 0;
}

int
vsscanf(const char *str, const char *fmt, va_list ap)
{
  char *s, *c, t, num[32];
  int n, *i;

  n = 0;
  while (*fmt != 0) {
    if (*fmt != '%') {
      if (*str != *fmt) {
	return n;
      } else {
	fmt++;
	str++;
      }
    }

    fmt++;
    t = *fmt;
    fmt++;

    switch (t) {
    case '%':
      if (*str != '%') {
	return n;
      } else {
	str++;
      }

      break;

    case 'i':
      i = va_arg(ap, int *);

      s = num;
      while (*str != *fmt) {
	*s++ = *str++;
      }

      *s = 0;

      *i = strtol(num, nil, 10);
      break;

    case 'c':
      c = va_arg(ap, char *);
      *c = *str;
      str++;
      break;

    case 's':
      s = va_arg(ap, char *);

      while (*str != *fmt) {
	*s++ = *str++;
      }

      *s = 0;
      break;

    default:
      return ERR;
    }

    n++;
  }

  return n;
}

int
fscanf(int fd, const char *fmt, ...)
{
  va_list ap;
  int i;

  va_start(ap, fmt);
  i = vfscanf(fd, fmt, ap);
  va_end(ap);

  return i;
}

int
scanf(const char *fmt, ...)
{
  va_list ap;
  int i;

  va_start(ap, fmt);
  i = vfscanf(STDIN, fmt, ap);
  va_end(ap);

  return i;
}

int
sscanf(const char *str, const char *fmt, ...)
{
  va_list ap;
  int i;

  va_start(ap, fmt);
  i = vsscanf(str, fmt, ap);
  va_end(ap);

  return i;
}

