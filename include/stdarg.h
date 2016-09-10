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

#ifndef _STDARG_H_
#define _STDARG_H_

/* Suprisingly this seems to work. */

typedef char* va_list;

#define va_start(ap, last)	(ap = (va_list) &last + sizeof(last))
#define va_end(ap)		(ap = 0)
#define va_arg(ap, type)	 *((type *) ap); ap += sizeof(type)

/*

typedef __builtin_va_list va_list;

#define va_start(ap, last)	__builtin_va_start((ap), last)
#define va_end(ap)		__builtin_va_end((ap))
#define va_arg(ap, type)	__builtin_va_arg((ap), type)

*/

#endif 
