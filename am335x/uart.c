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
#include "fns.h"

#define UART0   	0x44E09000

#define UART_LSR	0x14
#define UART_THR	0x00

void
putc(char c)
{
	if (c == '\n')
		putc('\r');
	
	while ((readl(UART0 + UART_LSR) & (1 << 5)) == 0);
	writel(c, UART0 + UART_THR);
}

void
puts(const char *c)
{
	while (*c)
		putc(*c++);
}
