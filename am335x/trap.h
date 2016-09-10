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

#ifndef __TRAP_H
#define __TRAP_H

#define MODE_SVC		19
#define MODE_FIQ		17
#define MODE_IRQ		18
#define MODE_ABORT		23
#define MODE_UND		27
#define MODE_SYS		31
#define MODE_USR		16

#define ABORT_INTERRUPT		0
#define ABORT_INSTRUCTION	1
#define ABORT_PREFETCH		2
#define ABORT_DATA	 	3

#endif
