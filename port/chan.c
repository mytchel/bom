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

struct chantype *chantypes[CHAN_max] = {
	[CHAN_pipe] = &devpipe,
	[CHAN_file] = &devfile,
};

struct chan *
newchan(int type, int mode, struct path *p)
{
	struct chan *c;
	
	c = malloc(sizeof(struct chan));
	if (c == nil) {
		return nil;
	}

	c->refs = 1;
	c->lock = 0;
	c->type = type;
	c->mode = mode;
	c->path = p;
	
	return c;
}

void
freechan(struct chan *c)
{
	lock(&c->lock);
	
	c->refs--;
	if (c->refs > 0) {
		unlock(&c->lock);
		return;
	}

	chantypes[c->type]->close(c);
	freepath(c->path);
	free(c);
}
