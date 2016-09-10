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

#include "../include/types.h"

void *
memmove(void *dest, const void *src, size_t len)
{
	uint8_t *d;
	const uint8_t *s;
	
	d = dest;
	s = src;
	
	while (len-- > 0)
		*d++ = *s++;
		
	return dest;
}

void *
memset(void *b, int c, size_t len)
{
	uint8_t *bb = b;
	
	while (len-- > 0)
		*bb++ = c;
		
	return b;
}

bool
strcmp(const uint8_t *s1, const uint8_t *s2)
{
	while (*s1 && *s2)
		if (*s1++ != *s2++)
			return false;
	
	if (*s1 == 0 && *s2 == 0)
		return true;
	else
		return false;
}

size_t
strlen(const uint8_t *s)
{
	size_t len = 0;
	
	while (*s++)
		len++;
	
	return len;
}
