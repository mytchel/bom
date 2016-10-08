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

struct binding *
newbinding(struct path *path, struct chan *out, struct chan *in)
{
  struct binding *b;
	
  b = malloc(sizeof(struct binding));
  if (b == nil)
    return nil;
	
  b->refs = 1;
  initlock(&b->lock);

  b->path = path;
  b->in = in;
  b->out = out;
  b->rootfid = ROOTFID;

  if (in != nil)
    atomicinc(&in->refs);

  if (out != nil)
    atomicinc(&out->refs);

  b->waiting = nil;
  b->nreqid = 0;
	
  return b;
}

void
freebinding(struct binding *b)
{
  atomicdec(&b->refs);
  /* 
   * Doesnt need to do any freeing
   * when mountproc sees that refs <=0
   * it will do freeing.
   */
}

/*
 * Find the binding that matches path to at most a depth of depth
 * return the best binding.
 */
 
struct binding *
findbinding(struct ngroup *ngroup, struct path *path, int depth)
{
  struct binding_list *bl;
  struct path *pp, *bp;
  struct binding *best;
  int d, bestd;
	
  lock(&ngroup->lock);

  best = nil;
  bestd = -1;
  for (bl = ngroup->bindings; bl != nil; bl = bl->next) {
    d = 0;
    pp = path;
    lock(&bl->binding->lock);
    bp = bl->binding->path;
    while (d < depth && pp != nil && bp != nil) {
      if (!strcmp(pp->s, bp->s)) {
	break;
      }
			
      d++;
      pp = pp->next;
      bp = bp->next;
    }
		
    if (bp == nil && d > bestd) {
      bestd = d;
      best = bl->binding;
    }
		
    unlock(&bl->binding->lock);
  }
	
  unlock(&ngroup->lock);
	
  return best;
}
