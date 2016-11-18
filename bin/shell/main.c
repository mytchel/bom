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
#include <stdarg.h>
#include <string.h>

#include "shell.h"

int ret = 0;

int
main(int argc, char *argv[])
{
  int fd, i, r;

  if (argc == 1) {
    interactive();
    return OK;
  } else {
    printf("processing files not yet implimented\n");
    return ERR;
    
    for (i = 1; i < argc; i++) {
      fd = open(argv[i], O_RDONLY);
      if (fd < 0) {
	printf("failed to open %s: %i\n", argv[i], fd);
	break;
      }

      close(fd);

      if (r != OK) {
	printf("error processing %s: %i\n", argv[i], r);
	break;
      }
    }
  }
}

void
interactive(void)
{
  char line[LINEMAX];
  struct atom *a;
  size_t r, i;

  while (true) {
    write(STDOUT, "% ", 2);

    i = 0;
    while ((r = read(STDIN, &line[i], 1)) > 0) {
      if (line[i] == '\n') {
	break;
      } else {
	i++;
      }
    }
    
    if (r == 0) {
      exit(OK);
    } else if (r < 0) {
      printf("error reading input: %i\n", r);
      exit(r);
    }

    line[i] = 0;
    a = parseatoml(line, i);
    if (a == nil) {
      exit(ERR);
    } else if (a->l.head == nil) {
      atomfree(a);
      continue;
    }

    ret = atomeval(a->l.head);

    atomfree(a);
  }
}
