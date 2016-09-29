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
newngroup(void)
{
  struct ngroup *n;
	
  n = malloc(sizeof(struct ngroup));
  if (n == nil) {
    return nil;
  }

  n->refs = 1;
  initlock(&n->lock);

  n->bindings = malloc(sizeof(struct binding_list));
  if (n->bindings == nil) {
    free(n);
    return nil;
  }

  n->bindings->binding = rootbinding;
  n->bindings->next = nil;

  atomicinc(&rootbinding->refs);
	
  return n;
}

void
freengroup(struct ngroup *n)
{
  struct binding_list *b, *bb;
	
  if (atomicdec(&n->refs) > 0) {
    return;
  }
		
  lock(&n->lock);
  b = n->bindings;
  while (b != nil) {
    bb = b;
    b = b->next;
    freebinding(bb->binding);
    free(bb);
  }

  free(n);
}

struct ngroup *
copyngroup(struct ngroup *o)
{
  struct ngroup *n;
  struct binding_list *bo, *bn;
	
  lock(&o->lock);
	
  n = malloc(sizeof(struct ngroup));
  if (n == nil) {
    unlock(&o->lock);
    return nil;
  }
	
  bo = o->bindings;
  if (bo != nil) {
    n->bindings = malloc(sizeof(struct binding_list));
    bn = n->bindings;
  } else {
    n->bindings = nil;
  }
	
  while (bo != nil) {
    lock(&bo->binding->lock);
		
    bn->binding = bo->binding;
    atomicinc(&bn->binding->refs);
			
    bo = bo->next;
    unlock(&bo->binding->lock);
		
    if (bo == nil) {
      bn->next = nil;
    } else {
      bn->next = malloc(sizeof(struct binding_list));
    }
		
    bn = bn->next;
		
  }
	
  n->refs = 1;
  initlock(&n->lock);
  unlock(&o->lock);
  return n;
}
