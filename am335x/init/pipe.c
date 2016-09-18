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

#include <libc.h>

int
ppipe0(int fd)
{
	int i, n, pid = getpid();
	
	printf("%i: ppipe0 started with fd %i\n", pid, fd);
	while (true) {
		sleep(0);
		printf("%i: wait for write\n", pid);
		n = read(fd, (void *) &i, sizeof(int));
		if (n != sizeof(int)) {
			printf("%i: failed to read. %i\n", pid, n);
			break;
		}
		
		sleep(0);
		printf("%i: read %i\n", pid, i);
	}
	
	printf("%i: exiting\n", pid);

	return 1;
}

int
ppipe1(int fd)
{
	int i, n, pid = getpid();
	printf("%i: ppipe1 started with fd %i\n", pid, fd);
	
	for (i = 0; i < 10; i++) {
		sleep(0);
		printf("%i: writing %i\n", pid, i);

		n = write(fd, (void *) &i, sizeof(int));
		if (n != sizeof(int)) {
			printf("%i: write failed. %i\n", pid, n);
			break;
		}
	}
	
	printf("%i: exiting\n", pid);

	return 2;
}
