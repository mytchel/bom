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
#include <string.h>
#include <fs.h>

int
main(int argc, char *argv[])
{
  uint8_t buf[512];
  int r, w, in, out;

  in = open(argv[1], O_RDONLY);
  if (in < 0) {
    printf("cp failed to open %s %i\n", argv[1], in);
    return in;
  }

  out = open(argv[2], O_WRONLY|O_CREATE, ATTR_rd|ATTR_wr);
  if (out < 0) {
    printf("cp failed to open %s %i\n", argv[2], out);
    return out;
  }

  while (true) {
    if ((r = read(in, buf, sizeof(buf))) < 0) {
      if (r == 0) {
	r = OK;
      } else {
	printf("cp failed to read %s %i\n", argv[1], r);
      }
      
      break;
    }

    if ((w = write(out, buf, r)) != r) {
      printf("cp failed to write %s %i\n", argv[2], w);
      r = w;
      break;
    }
  }

  close(in);
  close(out);

  return r;
}
