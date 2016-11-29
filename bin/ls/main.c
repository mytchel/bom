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
#include <mem.h>
#include <string.h>

int
lsh(char *filename, bool a)
{
  uint8_t buf[NAMEMAX * 3], len;
  struct stat s;
  int r, fd, i, e;

  if ((e = stat(filename, &s)) != OK) {
    printf("Error statting %s: %i\n", filename, e);
    return e;
  }

  if (!(s.attr & ATTR_dir)) {
    printf("%b\t%u\t%u\t%s\n", s.attr, s.size, s.dsize, filename);
    return OK;
  }

  if ((e = chdir(filename)) != OK) {
    printf("error descending into %s: %i\n", filename, e);
    return e;
  }

  if ((fd = open(".", O_RDONLY)) < 0) {
    printf("error opening %s: %i\n", filename, fd);
    return fd;
  }

  if (a) {
    printf("%s:\n", filename);
  }

  i = 0;
  while ((r = read(fd, &buf[i], sizeof(buf) - i)) > 0) {
    r += i;
    i = 0;
    while (i < r) {
      len = buf[i];

      if (i + 1 + len > r) {
	/* copy remander of buf to start */
	memmove(buf, &buf[i], r - i);
	i = r - i;
	break;
      }

      i++;

      if ((e = stat((char *) &buf[i], &s)) != OK) {
	printf("error stating %s:\n", &buf[i], e);
      } else {
	printf("%b\t%u\t%u\t%s\n", s.attr, s.size, s.dsize, &buf[i]);
      }

      i += len;
    }

    if (r == i) {
      i = 0;
    }
  }

  close(fd);

  return OK;
}

int
main(int argc, char **argv)
{
  int i, p, r;

  if (argc == 1) {
    r = lsh(".", false);
  } else if (argc == 2) {
    r = lsh(argv[1], false);
  } else {
    for (i = 1; i < argc; i++) {
      p = fork(FORK_sngroup|FORK_smem|FORK_sfgroup);
      if (p == 0) {
	r = lsh(argv[i], true);
	exit(r);
      } else {
	while (wait(&r) != p)
	  ;

	if (r != OK) {
	  return r;
	}
      }
    }
  }
  
  return r;
}
