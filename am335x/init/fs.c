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

#include "../include/libc.h"
#include "../include/fs.h"

int
pfile_open(void)
{
	int i;
	int fd;
	uint8_t c;
	uint8_t *str1 = (uint8_t *) "Hello, World\n";
	uint8_t *str2 = (uint8_t *) "How are you?\n";
	uint8_t *str3 = (uint8_t *) "This is just a test";

	write(stdout, str1, sizeof(uint8_t) * strlen(str1));
	
	/* Some tests */
	printf("open /tmp/test\n");
	fd = open("/tmp/test", O_WRONLY|O_CREATE, ATTR_wr|ATTR_rd);
	if (fd < 0) {
		printf("failed to open /tmp/test\n");
	} else {
		printf("write message\n");
		write(fd, str3, sizeof(uint8_t) * strlen(str3));
		close(fd);
	}
	
	printf("write to stdout a few times\n");
	for (i = 0; i < 5; i++) {
		write(stdout, str2, sizeof(uint8_t) * strlen(str2));
		sleep(700);
	}

	printf("open /tmp/test\n");
	fd = open("/tmp/test", O_RDONLY);
	if (fd < 0) {
		printf("failed to open /tmp/test\n");
	} else {
		printf("read /tmp/test\n");
		while (true) {
			i = read(fd, &c, sizeof(c));
			if (i != sizeof(c))
				break;

			printf("%i, '%c'\n", i, c);
		}
		
		printf("close and remove /tmp/test\n");
		
		close(fd);
		remove("/tmp/test");
	}
	
	return 0;
}
