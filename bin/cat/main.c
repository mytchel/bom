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

int
main(int argc, char *argv[])
{
  uint8_t buf[512];
  int i, fd, r, w;

  if (argc == 1) {
    while ((r = read(STDIN, buf, sizeof(buf))) > 0) {
      if ((w = write(STDOUT, buf, r)) != r) {
	printf("failed to write : %i\n", w);
	return w;
      }
    }

    if (r < 0) {
      printf("failed to read stdin %i\n", r);
      return r;
    }
  } else {
    for (i = 1; i < argc; i++) {
      fd = open(argv[i], O_RDONLY);
      if (fd < 0) {
	printf("cat %s failed %i\n", argv[i], fd);
	return fd;
      } else {
	while ((r = read(fd, buf, sizeof(buf))) > 0) {
	  if ((w = write(STDOUT, buf, r)) != r) {
	    printf("failed to write : %i\n", w);
	    return w;
	  }
	}

	if (r < 0) {
	  printf("failed to read %s : %i\n", argv[i], r);
	  return r;
	}

	close(fd);
      }
    }
  }

  return OK;
}
