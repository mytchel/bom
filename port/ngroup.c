/*
 *   Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
	n->lock = 0;
	n->bindings = malloc(sizeof(struct binding_list));
	if (n->bindings == nil) {
	  free(n);
	  return nil;
	}

	n->bindings->binding = rootbinding;
	n->bindings->next = nil;
	
	return n;
}

void
freengroup(struct ngroup *n)
{
	struct binding_list *b, *bb;
	
	lock(&n->lock);
	
	n->refs--;
	if (n->refs > 0) {
		unlock(&n->lock);
		return;
	}
		
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
		bn->binding->refs++;
			
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
	n->lock = 0;
	unlock(&o->lock);
	return n;
}
