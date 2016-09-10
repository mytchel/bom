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

static struct path *
strtopathh(struct path *prev, const uint8_t *str)
{
	struct path *p;
	int i, j;
	
	for (i = 0; str[i] && str[i] != '/'; i++);
	if (i == 0 && str[i]) {
		return strtopathh(prev, str + 1);
	} else if (i == 0) {
		return nil;
	}
	
	p = malloc(sizeof(struct path));
	/* Best to cause some more major issue in the case of a failed
	 * malloc, as I have no way to check if there was an error
	 * (or end of the path).
	 */
	
	p->s = malloc(sizeof(uint8_t) * (i+1));
	for (j = 0; j < i; j++)
		p->s[j] = str[j];
	p->s[j] = 0;
	
	p->prev = prev;
	p->next = strtopathh(p, str + i);
	return p;
}

struct path *
strtopath(const uint8_t *str)
{
	return strtopathh(nil, str);
}

uint8_t *
pathtostr(struct path *p, int *n)
{
	uint8_t *str;
	size_t len, i;
	struct path *pp;
	
	len = 0;
	if (p == nil) {
		len = 1;
	}
	
	for (pp = p; pp != nil; pp = pp->next)
		len += strlen(pp->s) + 1;

	str = malloc(sizeof(uint8_t) * len);
	if (str == nil) {
		return nil;
	}
		
	if (n != nil)
		*n = len;
		
	i = 0;
	for (pp = p; pp != nil; pp = pp->next) {
		len = strlen(pp->s);
		memmove(&str[i], pp->s, len);
		i += len;
		if (pp->next)
			str[i++] = '/';
	}
	
	str[i] = 0;
		
	return str;
}

struct path *
realpath(struct path *po, const uint8_t *str)
{
	struct path *pstr, *pp, *pt, *path;

	pstr = strtopath(str);
	if (str[0] == '/' || po == nil) {
		path = pstr;
	} else {
		path = copypath(po);

		for (pp = path; pp != nil && pp->next != nil; pp = pp->next);

		pp->next = pstr;
	}
	
	pp = path;
	while (pp != nil) {
		if (strcmp(pp->s, (const uint8_t *) "..")) {
			/* Remove current and previous part */
			pt = pp->prev;
			if (pt == nil) {
				path = pp->next;
				if (pp->next != nil)
					pp->next->prev = nil;
				
				free(pp->s);
				free(pp);
				pp = path;
			} else {
				if (pt->prev != nil) {
					pt->prev->next = pp->next;
					if (pp->next != nil)
						pp->next->prev = pt->prev;
				} else {
					path = pp->next;
					if (pp->next != nil)
						pp->next->prev = nil;
				}
				
				free(pt->s);
				free(pt);

				pt = pp;
				pp = pp->next;
				free(pt->s);
				free(pt);
			}	
		} else if (strcmp(pp->s, (const uint8_t *) ".")) {
			/* Remove current part */
			pt = pp;
			
			if (pp->prev != nil) {
				pp->prev->next = pp->next;
			} else {
				path = pp->next;
			}
			
			if (pp->next != nil)
				pp->next->prev = pp->prev;
			
			pp = pp->next;
			
			free(pt->s);
			free(pt);
		} else {
			pp = pp->next;
		}
	}
	
	return path;
}

struct path *
copypath(struct path *o)
{
	struct path *n, *nn;
	size_t l;
	
	if (o == nil)
		return nil;
	
	n = malloc(sizeof(struct path));
	nn = n;
	nn->prev = nil;
	
	while (o != nil) {
		l = strlen(o->s) + 1;
		nn->s = malloc(sizeof(uint8_t) * l);
		memmove(nn->s, o->s, l);

		o = o->next;
		if (o != nil) {
			nn->next = malloc(sizeof(struct path));
			nn->next->prev = nn;
			nn = nn->next;
		}
	}
	
	nn->next = nil;
	
	return n;
}

void
freepath(struct path *p)
{
	if (p == nil)
		return;
		
	freepath(p->next);
	
	free(p->s);
	free(p);
}
