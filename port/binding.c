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

struct binding *
newbinding(struct path *path, struct chan *out, struct chan *in)
{
	struct binding *b;
	
	b = malloc(sizeof(struct binding));
	if (b == nil)
		return nil;
	
	b->refs = 1;
	b->lock = 0;

	b->path = path;
	b->in = in;
	b->out = out;
	b->rootfid = ROOTFID;
	
	in->refs++;
	out->refs++;

	b->waiting = nil;
	b->nreqid = 0;
	
	return b;
}

void
freebinding(struct binding *b)
{
	lock(&b->lock);
	
	b->refs--;
	
	unlock(&b->lock);
	
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
