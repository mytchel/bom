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

#include "matrix.h"

/* Attaching all the grounds, 3V, and 5V seems to help.
   At least one of each is defiantly necessary */

struct pin pins[NPINS] = {     /* raspberry */
  [R1]  = { "/dev/gpio2",   2, 0 }, /* gpio5    29 */
  [G1]  = { "/dev/gpio2",   5, 0 }, /* gpio13   33 */
  [B1]  = { "/dev/gpio1",  13, 0 }, /* gpio6    31 */
  [R2]  = { "/dev/gpio0",  23, 0 }, /* gpio12   32 */
  [G2]  = { "/dev/gpio1",  15, 0 }, /* gpio16   36 */
  [B2]  = { "/dev/gpio0",  27, 0 }, /* gpio23   16 */
  [OE]  = { "/dev/gpio0",  22, 0 }, /* gpio4     7 */
  [CLK] = { "/dev/gpio2",   3, 0 }, /* gpio17   11 */
  [LAT] = { "/dev/gpio2",   4, 0 }, /* gpio21   40 */
  [RA]  = { "/dev/gpio1",  12, 0 }, /* gpio22   15 */
  [RB]  = { "/dev/gpio0",  26, 0 }, /* gpio26   37 */
  [RC]  = { "/dev/gpio1",  14, 0 }, /* gpio27   13 */
  [RD]  = { "/dev/gpio2",   1, 0 }, /* gpio20   38 */
};

int
setpin(pin_t p, bool on)
{
  char *msg;
  int r, l;

  if (pins[p].level == on) {
    return OK;
  }
  
  if (on) {
    l = pins[p].highlen;
    msg = pins[p].high;
  } else {
    l = pins[p].lowlen;
    msg = pins[p].low;
  }
  
  pins[p].level = on;
  r = write(pins[p].fd, msg, l);
  if (r != l) {
    printf("error writing '%s' > %s : %i\n", msg, pins[p].dev, r);
    return ERR;
  } else {
    return OK;
  }
}

int
setuppins(void)
{
  char msg[32];
  size_t l;
  pin_t p;
  int r;

  for (p = 0; p < NPINS; p++) {
    pins[p].fd = open(pins[p].dev, O_WRONLY);
    if (pins[p].fd < 0) {
      printf("failed to open %s : %i\n", pins[p].dev, pins[p].fd);
      return ERR;
    }

    l = snprintf(msg, sizeof(msg), "%i out", pins[p].num);
    r = write(pins[p].fd, msg, l);
    if (r != l) {
      printf("failed to set %s:%i out\n", pins[p].dev, pins[p].num);
      return ERR;
    }

    pins[p].lowlen = snprintf(pins[p].low, sizeof(pins[p].low),
			      "%i low", pins[p].num);
    pins[p].highlen = snprintf(pins[p].high, sizeof(pins[p].high),
			       "%i high", pins[p].num);

    pins[p].level = true;
    setpin(p, false);
  }

  return OK;
}

void
reset(void)
{
  setpin(OE, false);
  setpin(LAT, false);

  setpin(RA, false);
  setpin(RB, false);
  setpin(RC, false);
  setpin(RD, false);

  setpin(R1, false);
  setpin(G1, false);
  setpin(B1, false);
  setpin(R2, false);
  setpin(G2, false);
  setpin(B2, false);
}

void
setrow(int row)
{
  if (row & (1 << 0)) {
    setpin(RA, true);
  } else {
    setpin(RA, false);
  }
      
  if (row & (1 << 1)) {
    setpin(RB, true);
  } else {
    setpin(RB, false);
  }

  if (row & (1 << 2)) {
    setpin(RC, true);
  } else {
    setpin(RC, false);
  }

  if (row & (1 << 3)) {
    setpin(RD, true);
  } else {
    setpin(RD, false);
  }
}

int
matrix(int delay, int color, int repeat)
{
  int e, row;

  e = setuppins();
  if (e != OK) {
    printf("failed to setup pins : %i\n", e);
    return e;
  }

  reset();

  if (color & (1 << 0)) {
    setpin(R1, true);
  } else {
    setpin(R1, false);
  }
      
  if (color & (1 << 1)) {
    setpin(G1, true);
  } else {
    setpin(G1, false);
  }

  if (color & (1 << 2)) {
    setpin(B1, true);
  } else {
    setpin(B1, false);
  }

  setpin(B2, true);
  setpin(R2, true);

  row = 0;
  while (true) {
    setpin(OE, true);

    setpin(LAT, true);
    setpin(LAT, false);

    setrow(row++);
    if (row == 16) {
      row = 0;
    }

    setpin(OE, false);

    for (e = 0; e < 32; e++) {
      setpin(RA, !pins[RA].level);
    }
  }
   
  return OK;
}
