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
#include <stdarg.h>

int
commount(char *path);

int
initblockdevs(void);

int
shell(void);

int stdin, stdout, stderr;

int
main(void)
{
  int f, fd, code;

  fd = open("/dev", O_WRONLY|O_CREATE,
	    ATTR_wr|ATTR_rd|ATTR_dir);
  if (fd < 0) {
    return -1;
  }

  f = commount("/dev/com");
  if (f < 0) {
    return -1;
  }

  stdin = open("/dev/com", O_RDONLY);
  stdout = open("/dev/com", O_WRONLY);
  stderr = open("/dev/com", O_WRONLY);

  if (stdin < 0) return -2;
  if (stdout < 0) return -3;
  if (stderr < 0) return -3;
 
  f = initblockdevs();
  if (f < 0) {
    return -1;
  }

  f = fork(FORK_sngroup);
  if (f == 0) {
    return shell();
  }

  while ((f = wait(&code)) > 0) {
    printf("%i exited with %i\n", f, code);
  }

  printf("should probably shutdown or something.\n");
  return OK;
}
