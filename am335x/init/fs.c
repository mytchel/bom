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

static void
readblock(const char *name)
{
  uint8_t buf[8];
  int i, fd;
  
  printf("open %s\n", name);
  fd = open(name, O_RDONLY);
  if (fd < 0) {
    printf("failed to open %s\n", name);
  } else {
    for (i = 0; i < 32; i++) {
      if (read(fd, buf, sizeof(buf)) != sizeof(buf))
	break;

      printf("%h%h  %h%h  %h%h  %h%h\n", buf[0], buf[1], buf[2],
	     buf[3], buf[4], buf[5], buf[6], buf[7]);
    }

    printf("closing %s\n", name);
    close(fd);
  }
}

int
filetest(void)
{
  int i;
  int fd;
  uint8_t c;
  uint8_t *str1 = (uint8_t *) "Hello, World\n";
  uint8_t *str2 = (uint8_t *) "How are you?\n";
  uint8_t *str3 = (uint8_t *) "This is just a test";

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

  printf("main test done\n");

  readblock("/dev/mmc0");
  readblock("/dev/mmc1");

  return 1;
}
