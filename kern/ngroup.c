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
    bindingfidfree(bb->boundfid);
    bindingfidfree(bb->rootfid);
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

    bn->boundfid = bo->boundfid;
    bn->rootfid = bo->rootfid;

    atomicinc(&bn->boundfid->refs);
    atomicinc(&bn->rootfid->refs);
			
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
bindingnew(struct chan *out, struct chan *in, uint32_t rootattr)
{
  struct binding *b;
	
  b = malloc(sizeof(struct binding));
  if (b == nil)
    return nil;
	
  b->fids = malloc(sizeof(struct bindingfid));
  if (b->fids == nil) {
    free(b);
    return nil;
  }

  b->fids->refs = 1;
  b->fids->binding = b;
  b->fids->parent = nil;
  b->fids->children = nil;
  b->fids->cnext = nil;
  b->fids->fid = ROOTFID;
  memset(b->fids->name, 0, NAMEMAX);
  b->fids->attr = rootattr;

  b->refs = 1;

  lockinit(&b->lock);

  b->in = in;
  b->out = out;

  if (in != nil) {
    atomicinc(&in->refs);
  }
  
  if (out != nil) {
    atomicinc(&out->refs);
  }

  b->waiting = nil;
  b->nreqid = 0;
	
  return b;
}

void
bindingfree(struct binding *b)
{
  if (atomicdec(&b->refs) > 0) {
    return;
  }

  /* There should only be the root binding left, which
   * bindingfidfree releasing after calling this. */

  free(b);
}

int
ngroupaddbinding(struct ngroup *n, struct binding *b,
		 struct bindingfid *boundfid,
		 struct bindingfid *rootfid)
{
  struct bindingl *bl;
  
  bl = malloc(sizeof(struct bindingl));
  if (bl == nil) {
    return ENOMEM;
  }

  atomicinc(&b->refs);
  
  bl->binding = b;
  bl->rootfid = rootfid;
  bl->boundfid = boundfid;

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

  bindingfidfree(bl->boundfid);
  bindingfidfree(bl->rootfid);

  free(bl);

  bindingfree(b);
}

struct bindingl *
ngroupfindbindingl(struct ngroup *ngroup, struct bindingfid *fid)
{
  struct bindingl *bl;
	
  lock(&ngroup->lock);

  for (bl = ngroup->bindings; bl != nil; bl = bl->next) {
    if (bl->boundfid == fid) {
      break;
    }
  }

  unlock(&ngroup->lock);
  return bl;
}
