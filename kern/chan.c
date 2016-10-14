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

#include "head.h"

extern struct chantype devpipe, devfile;

struct chantype *chantypes[CHAN_max] = {
  [CHAN_pipe] = &devpipe,
  [CHAN_file] = &devfile,
};

struct chan *
channew(int type, int mode)
{
  struct chan *c;
	
  c = malloc(sizeof(struct chan));
  if (c == nil) {
    return nil;
  }

  lockinit(&c->lock);
  c->refs = 1;
  c->type = type;
  c->mode = mode;
	
  return c;
}

void
chanfree(struct chan *c)
{
  if (atomicdec(&c->refs) > 0) {
    return;
  }

  chantypes[c->type]->close(c);
  free(c);
}
