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
fgroupnew(void)
{
  struct fgroup *f;
	
  f = malloc(sizeof(struct fgroup));
  if (f == nil) {
    return nil;
  }

  f->chans = malloc(sizeof(struct chan*));
  if (f->chans == nil) {
    free(f);
    return nil;
  }
	
  f->chans[0] = nil;
  f->nchans = 1;
  f->refs = 1;
  lockinit(&f->lock);
	
  return f;
}

void
fgroupfree(struct fgroup *f)
{
  int i;
	
  if (atomicdec(&f->refs) > 0) {
    return;
  }

  printf("close %i chans\n", f->nchans);
  for (i = 0; i < f->nchans; i++) {
    if (f->chans[i] != nil) {
      printf("close fd %i\n", i);
      chanfree(f->chans[i]);
    }
  }

  printf("free chans\n");
  free(f->chans);
  free(f);
}

struct fgroup *
fgroupcopy(struct fgroup *fo)
{
  int i;
  struct fgroup *f;
	
  lock(&fo->lock);
	
  f = malloc(sizeof(struct fgroup));
  if (f == nil) {
    unlock(&fo->lock);
    return nil;
  }

  f->chans = malloc(sizeof(struct chan*) * fo->nchans);
  if (f->chans == nil) {
    free(f);
    unlock(&fo->lock);
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
  lockinit(&f->lock);

  unlock(&fo->lock);
  return f;
}

int
fgroupaddchan(struct fgroup *f, struct chan *chan)
{
  int i, fd;
	
  lock(&f->lock);

  for (i = 0; i < f->nchans; i++)
    if (f->chans[i] == nil)
      break;
	
  if (i < f->nchans) {
    fd = i;
  } else {
    /* Need to grow file table. */
    struct chan **chans;
		
    chans = malloc(sizeof(struct chan **)
		   * (f->nchans * 2));

    if (chans == nil) {
      unlock(&f->lock);
      return -1;
    }
		
    for (i = 0; i < f->nchans; i++)
      chans[i] = f->chans[i];

    fd = i++;

    while (i < f->nchans * 2)
      chans[i++] = nil;

    free(f->chans);
				
    f->chans = chans;
    f->nchans *= 2;
  }

  f->chans[fd] = chan;
  unlock(&f->lock);

  return fd;
}

struct chan *
fdtochan(struct fgroup *f, int fd)
{
  struct chan *c = nil;

  lock(&f->lock);
	
  if (fd >= 0 && fd < f->nchans)
    c = f->chans[fd];
	
  unlock(&f->lock);

  return c;
}
