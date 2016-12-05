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
machinit(void)
{
  int f;
  
  f = commount("/dev/com");
  if (f < 0) {
    return ERR;
  }

  if (open("/dev/com", O_RDONLY) != STDIN) {
    return ERR;
  } else if (open("/dev/com", O_WRONLY) != STDOUT) {
    return ERR;
  } else if (open("/dev/com", O_WRONLY) != STDERR) {
    return ERR;
  }

  f = initgpios();
  if (f < 0) {
    printf("initgpios failed!\n");
    return ERR;
  }

  f = initblockdevs();
  if (f < 0) {
    printf("initblockdevs failed!\n");
    return ERR;
  }

  return OK;
}
