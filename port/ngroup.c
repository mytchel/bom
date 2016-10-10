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

struct ngroup *
ngroupnew(void)
{
  struct ngroup *n;
	
  n = malloc(sizeof(struct ngroup));
  if (n == nil) {
    return nil;
  }

  n->refs = 1;
  lockinit(&n->lock);

  n->bindings = nil;
	
  return n;
}

void
ngroupfree(struct ngroup *n)
{
  struct bindingl *b, *bb;
	
  if (atomicdec(&n->refs) > 0) {
    return;
  }
		
  b = n->bindings;
  while (b != nil) {
    bb = b;
    b = b->next;
    bindingfree(bb->binding);
    pathfree(bb->path);
    free(bb);
  }

  free(n);
}

struct ngroup *
ngroupcopy(struct ngroup *o)
{
  struct ngroup *n;
  struct bindingl *bo, *bn;
	
  lock(&o->lock);
	
  n = malloc(sizeof(struct ngroup));
  if (n == nil) {
    unlock(&o->lock);
    return nil;
  }
	
  bo = o->bindings;
  if (bo != nil) {
    n->bindings = malloc(sizeof(struct bindingl));
    bn = n->bindings;
  } else {
    n->bindings = nil;
  }
	
  while (bo != nil) {
    bn->binding = bo->binding;
    atomicinc(&bn->binding->refs);

    bn->path = pathcopy(bo->path);
    bn->rootfid = bo->rootfid;
			
    bo = bo->next;

    if (bo == nil) {
      bn->next = nil;
    } else {
      bn->next = malloc(sizeof(struct bindingl));
    }
		
    bn = bn->next;
		
  }

  unlock(&o->lock);
	
  n->refs = 1;
  lockinit(&n->lock);

  return n;
}

struct binding *
bindingnew(struct chan *out, struct chan *in)
{
  struct binding *b;
	
  b = malloc(sizeof(struct binding));
  if (b == nil)
    return nil;
	
  b->refs = 1;

  lockinit(&b->lock);

  b->in = in;
  b->out = out;

  if (in != nil)
    atomicinc(&in->refs);

  if (out != nil)
    atomicinc(&out->refs);

  b->waiting = nil;
  b->nreqid = 0;
	
  return b;
}

void
bindingfree(struct binding *b)
{
  atomicdec(&b->refs);
  /* 
   * Doesnt need to do any freeing
   * when mountproc sees that refs <=0
   * it will do freeing.
   */
}

int
ngroupaddbinding(struct ngroup *n, struct binding *b,
		 struct path *p, uint32_t rootfid)
{
  struct bindingl *bl;
  
  bl = malloc(sizeof(struct bindingl));
  if (bl == nil) {
    return ENOMEM;
  }
		
  bl->binding = b;
  bl->rootfid = rootfid;
  bl->path = p;

  lock(&n->lock);

  bl->next = n->bindings;
  n->bindings = bl;

  unlock(&n->lock);

  return OK;
}

void
ngroupremovebinding(struct ngroup *n, struct binding *b)
{
  struct bindingl *bl, *prev;

  lock(&n->lock);
  prev = nil;

  for (bl = n->bindings;
       bl != nil && bl->binding != b;
       prev = bl, bl = bl->next)
    ;

  if (prev == nil) {
    prev->next = bl->next;
  } else {
    n->bindings = bl->next;
  }

  unlock(&n->lock);

  pathfree(bl->path);
  free(bl);
}

/*
 * Find the binding that matches path to at most a depth of depth
 * return the best binding.
 */
 
struct binding *
ngroupfindbinding(struct ngroup *ngroup, struct path *path,
		  int depth, uint32_t *rfid)
{
  struct bindingl *bl, *best;
  struct path *pp, *bp;
  int d, bestd;
	
  lock(&ngroup->lock);

  best = nil;
  bestd = -1;
  for (bl = ngroup->bindings; bl != nil; bl = bl->next) {
    d = 0;
    pp = path;

    bp = bl->path;
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
      best = bl;
    }
  }
	
  unlock(&ngroup->lock);

  *rfid = best->rootfid;
  return best->binding;
}
