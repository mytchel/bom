/*
 *
 * Copyright (c) <2016> Mytchel Hammond <mytchel@openmailbox.org>
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

int
ppipe0(int fd)
{
  int i, n, pid = getpid();
	
  printf("ppipe0 %i: ppipe0 started with fd %i\n", pid, fd);
  while (true) {
    n = read(fd, (void *) &i, sizeof(int));
    if (n != sizeof(int)) {
      printf("ppipe0 %i: failed to read. %i\n", pid, n);
      break;
    }
		
    printf("ppipe0 %i: read %i\n", pid, i);
  }
	
  printf("ppipe0 %i: exiting\n", pid);

  return 1;
}

int
ppipe1(int fd)
{
  int i, n, pid = getpid();
  printf("ppipe1 %i: ppipe1 started with fd %i\n", pid, fd);
	
  for (i = 0; i < 10; i++) {
    printf("ppipe1 %i: writing %i\n", pid, i);

    n = write(fd, (void *) &i, sizeof(int));
    if (n != sizeof(int)) {
      printf("ppipe1 %i: write failed. %i\n", pid, n);
      break;
    }
  }
	
  printf("ppipe1 %i: exiting\n", pid);

  return 2;
}
