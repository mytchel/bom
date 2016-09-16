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

void
printf(const char *fmt, ...)
{
	char str[512];
	va_list ap;
	
	va_start(ap, fmt);
	vsnprintf(str, 512, fmt, ap);
	va_end(ap);
	
	puts(str);
}

void
panic(const char *fmt, ...)
{
	char str[512];
	va_list ap;

	printf("\n\nPanic!\n\n");
	
	va_start(ap, fmt);
	vsnprintf(str, 512, fmt, ap);
	va_end(ap);
	
	puts(str);

	printf("\n\nHanging...\n");
	disableintr();
	while (1);
}
