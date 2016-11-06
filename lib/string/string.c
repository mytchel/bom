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
#include <types.h>
#include <stdarg.h>
#include <string.h>

bool
strncmp(const char *s1, const char *s2, size_t len)
{
  while (len-- > 0) {
    if (*s1 == 0 && *s2 == 0) {
      return true;
    } else if (*s1++ != *s2++) {
      return false;
    }
  }

  return true;
}

bool
strcmp(const char *s1, const char *s2)
{
  size_t l1, l2;

  l1 = strlen(s1);
  l2 = strlen(s2);

  if (l1 == l2) {
    return strncmp(s1, s2, l1);
  } else {
    return false;
  }
}

size_t
strlen(const char *s)
{
  size_t len = 0;
	
  while (*s++)
    len++;
	
  return len;
}

size_t
strlcpy(char *dest, const char *src, size_t max)
{
  size_t i;
  
  for (i = 1; i < max - 1 && *src != 0; i++) {
    *dest++ = *src++;
  }

  *dest = 0;
  return i;
}

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

char *
strtok(char *nstr, const char *sep)
{
  static char *str;
  int i, seplen;

  if (nstr != nil) {
    str = nstr;
  } else if (str == nil) {
    return nil;
  }
  
  nstr = str;
  seplen = strlen(sep);
  
  i = 0;
  while (str[i]) {
    if (strncmp(&str[i], sep, seplen)) {
      break;
    } else {
      i++;
    }
  }

  if (str[i]) {
    str[i++] = 0;
    while (str[i]) {
      if (!strncmp(&str[i], sep, seplen)) {
	break;
      } else {
	i += seplen;
      }
    }
 
    str = &str[i];
  } else {
    str = nil;
  }

  return nstr;
}
