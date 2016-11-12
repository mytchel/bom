/*
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
#include <fs.h>
#include <stdarg.h>
#include <string.h>

#include "shell.h"

static int
readsentence(uint8_t *data, size_t max)
{
  size_t i;
  char c;

  i = 0;
  while (i < max) {
    if (read(STDIN, &c, sizeof(char)) < 0) {
      return -1;
    } else if (c == '\n' || c == ';') {
      data[i] = '\0';
      return i;
    } else {
      data[i++] = c;
    }
  }

  data[i-1] = 0;
  return i;
}

int
main(int argc, char *argv[])
{
  uint8_t line[LINE_MAX] = {0};

  while (true) {
    printf("%% ");
    readsentence(line, LINE_MAX);
    processsentence((char *) line);
  }

  /* Never reached */
  return ERR;
}
