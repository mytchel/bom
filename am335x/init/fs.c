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
#include <fs.h>
#include <stdarg.h>
#include <string.h>

int
filetest(void)
{
  int i;
  int fd;
  uint8_t c;
  char *str1 = "Hello, World\n";
  char *str2 = "How are you?\n";
  char *str3 = "This is just a test";

  write(stdout, str1, strlen(str1));
	
  /* Some tests */
  printf("open /tmp/test\n");
  fd = open("/tmp/test", O_WRONLY|O_CREATE, ATTR_wr|ATTR_rd);
  if (fd < 0) {
    printf("failed to open /tmp/test\n");
  } else {
    printf("write message\n");
    write(fd, str3, strlen(str3));
    close(fd);
  }

  printf("write to stdout a few times\n");
  for (i = 0; i < 5; i++) {
    write(stdout, str2, strlen(str2));
    sleep(100);
  }

  printf("open /tmp/test\n");
  fd = open("/tmp/test", O_RDONLY);
  if (fd < 0) {
    printf("failed to open /tmp/test\n");
  } else {
    printf("read /tmp/test\n");
    while (true) {
      i = read(fd, &c, sizeof(c));
      if (i != sizeof(c))
	break;

      printf("%i, '%c'\n", i, c);
    }
		
    printf("close /tmp/test\n");
    close(fd);
    printf("remove /tmp/test\n");
    fd = remove("/tmp/test");
    if (fd < 0) {
      printf("/tmp/test remove failed!\n");
    } else {
      printf("/tmp/test removed\n");
    }
  }

  return 1;
}
