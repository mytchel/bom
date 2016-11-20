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

struct fgroup *
fgroupnew(size_t max)
{
  struct fgroup *f;
	
  f = malloc(sizeof(struct fgroup));
  if (f == nil) {
    return nil;
  }

  f->nchans = max;
  f->chans = malloc(sizeof(struct chan *) * max);
  if (f->chans == nil) {
    free(f);
    return nil;
  }

  memset(f->chans, 0, sizeof(struct chan *) * max);

  f->refs = 1;
	
  return f;
}

void
fgroupfree(struct fgroup *f)
{
  int i;
	
  if (atomicdec(&f->refs) > 0) {
    return;
  }

  for (i = 0; i < f->nchans; i++) {
    if (f->chans[i] != nil) {
      chanfree(f->chans[i]);
    }
  }

  free(f->chans);
  free(f);
}

struct fgroup *
fgroupcopy(struct fgroup *fo)
{
  struct fgroup *f;
  int i;
	
  f = malloc(sizeof(struct fgroup));
  if (f == nil) {
    return nil;
  }

  f->chans = malloc(sizeof(struct chan *) * fo->nchans);
  if (f->chans == nil) {
    free(f);
    return nil;
  }
	
  f->nchans = fo->nchans;
  for (i = 0; i < f->nchans; i++) {
    f->chans[i] = fo->chans[i];
    if (f->chans[i] != nil) {
      atomicinc(&f->chans[i]->refs);
    }
  }
	
  f->refs = 1;

  return f;
}

int
fgroupaddchan(struct fgroup *f, struct chan *chan)
{
  int i;
	
  for (i = 0; i < f->nchans; i++) {
    if (f->chans[i] == nil) {
      if (cas(&f->chans[i], nil, chan)) {
	return i;
      } else {
	return fgroupaddchan(f, chan);
      }
    }
  }

  return ERR;
}

int
fgroupreplacechan(struct fgroup *f, struct chan *chan, int fd)
{
  struct chan *c;

  if (fd >= f->nchans) {
    return ERR;
  }

  do {
    c = f->chans[fd];
  } while (!cas(&f->chans[fd], c, chan));

  if (c != nil) {
    chanfree(c);
  }

  return fd;
}

struct chan *
fdtochan(struct fgroup *f, int fd)
{
  if (fd >= 0 && fd < f->nchans) {
    return f->chans[fd];
  } else {
    return nil;
  }
}
