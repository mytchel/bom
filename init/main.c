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
#include <fs.h>
#include <string.h>
#include <mem.h>

int
commount(char *path);

int
initblockdevs(void);

int
initgpios(void);

int
mountfat(char *device, char *dir);

void
interp(void);

int
matrix(int delay, int color, int repeat);

static int
readline(char *data, size_t max)
{
  size_t i;
  char c;
  int r;

  i = 0;
  while (i < max) {
    r = read(STDIN, &c, sizeof(char));
    if (r <= 0) {
      return -1;
    } else if (c == '\n') {
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
  int r, f, fd;
  char root[NAMEMAX * 5];

  fd = open("/dev", O_RDONLY|O_CREATE,
	    ATTR_wr|ATTR_rd|ATTR_dir);

  if (fd < 0) {
    return -1;
  }

  close(fd);

  fd = open("/mnt", O_RDONLY|O_CREATE,
	    ATTR_rd|ATTR_dir);

  if (fd < 0) {
    return -1;
  }

  close(fd);

  fd = open("/bin", O_RDONLY|O_CREATE,
	    ATTR_rd|ATTR_dir);

  if (fd < 0) {
    return -1;
  }

  close(fd);

  f = commount("/dev/com");
  if (f < 0) {
    return -1;
  }

  if (open("/dev/com", O_RDONLY) != STDIN) {
    return -2;
  } else if (open("/dev/com", O_WRONLY) != STDOUT) {
    return -2;
  } else if (open("/dev/com", O_WRONLY) != STDERR) {
    return -2;
  }

  f = initblockdevs();
  if (f < 0) {
    printf("initblockdevs failed!\n");
    return -3;
  }

  f = initgpios();
  if (f < 0) {
    printf("initgpios failed!\n");
    return -4;
  }

  /*
  matrix(10000, 1|(1<<1)|(1<<2), 1);
  */
  
  while (true) {
    printf("root: ");

    r = readline(root, sizeof(root));
    if (r < 0) {
      printf("error reading!\n");
      return ERR;
    }

    printf("mountfat %s /mnt\n", root);
    r = mountfat(root, "/mnt");
    if (r == OK) {
      break;
    } else {
      printf("error mounting root!\n");
    }
  }

  printf("bind /mnt/bin /bin\n");
  bind("/mnt/bin", "/bin");
  
  interp();
  /*
  printf("exec /bin/init\n");
  r = exec("/bin/init", argc, argv);

  printf("error exec /bin/init: %i\n", r);
  */
  
  return r;
}
