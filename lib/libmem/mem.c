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

void *
memmove(void *dest, const void *src, size_t len)
{
  uint8_t *d;
  const uint8_t *s;
	
  d = dest;
  s = src;

  while (len-- > 0) {
    *d++ = *s++;
  }
		
  return dest;
}

void *
memset(void *dest, int c, size_t len)
{
  uint8_t *bb = dest;
	
  while (len-- > 0)
    *bb++ = c;
		
  return dest;
}

uint16_t
intcopylittle16(uint8_t *src)
{
  size_t offset = 0;
  uint16_t d = 0;

  while (offset < sizeof(d)) {
    d |= *src++ << (offset++ * 8);
  }

  return d;
}

uint32_t
intcopylittle32(uint8_t *src)
{
  size_t offset = 0;
  uint32_t d = 0;

  while (offset < sizeof(d)) {
    d |= *src++ << (offset++ * 8);
  }

  return d;
}

uint16_t
intcopybig16(uint8_t *src)
{
  uint16_t d = 0;
  size_t offset = sizeof(d);

  while (offset-- > 0) {
    d |= *src++ << (offset * 8);
  }

  return d;
}

uint32_t
intcopybig32(uint8_t *src)
{
  uint32_t d = 0;
  size_t offset = sizeof(d);
  
  while (offset-- > 0) {
    d |= *src++ << (offset * 8);
  }

  return d;
}

void
intwritelittle16(uint8_t *dest, uint16_t v)
{
  size_t offset;

  for (offset = 0; offset < sizeof(v) * 8; offset += 8) {
    *dest++ = (v >> offset) & 0xff;
  }
}

void
intwritelittle32(uint8_t *dest, uint32_t v)
{
  size_t offset;

  for (offset = 0; offset < sizeof(v) * 8; offset += 8) {
    *dest++ = (v >> offset) & 0xff;
  }
}

