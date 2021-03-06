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

#include "head.h"

static struct lock pl;

int
printf(const char *fmt, ...)
{
  char str[512];
  va_list ap;
  int i;
	
  va_start(ap, fmt);
  i = vsnprintf(str, 512, fmt, ap);
  va_end(ap);

  if (i > 0) {
    lock(&pl);
    puts(str);
    unlock(&pl);
  }

  return i;
}

void
panic(const char *fmt, ...)
{
  char str[512];
  va_list ap;

  printf("\n\nPanic!\n\n");
	
  va_start(ap, fmt);
  vsnprintf(str, 512, fmt, ap);
  va_end(ap);
	
  puts(str);

  printf("\n\nHanging...\n");
  setintr(INTR_off);
  while (1)
    ;
}
