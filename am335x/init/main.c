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
#include "../include/stdarg.h"
#include "../include/fs.h"

int
ppipe0(int fd);

int
ppipe1(int fd);

int
mountloop(int, int);

int
pfile_open(void);

bool
uartinit(void);

int
mmc(void);

int stdin, stdout, stderr;

int
main(void)
{
	int f;
	int p1[2], p2[2];
	int fds[2];

	if (pipe(p1) == ERR) {
		return -4;
	} else if (pipe(p2) == ERR) {
		return -5;
	}
	
	if (bind(p1[1], p2[0], "/") < 0) {
		return -6;
	}
	
	close(p1[1]);
	close(p2[0]);

	f = fork(FORK_sngroup);
	if (f < 0) {
		return -1;
	} else if (!f) {
		return mountloop(p1[0], p2[1]);
	}
	
	close(p1[0]);
	close(p2[1]);
	
	stdin = open("/dev/com", O_RDONLY);
	stdout = open("/dev/com", O_WRONLY);
	stderr = open("/dev/com", O_WRONLY);

	if (stdin < 0) return -1;
	if (stdout < 0) return -2;
	if (stderr < 0) return -3;
	
	printf("Starting World\n");
	
	printf("in, out, err = %i, %i, %i\n", stdin, stdout, stderr);
	
	printf("/ mount proc is pid %i\n", f);
	
	printf("pfile_open\n");
	f = fork(FORK_sngroup);
	if (f < 0) {
		return -2;
	} else if (!f) {
		printf("forked to %i\n", getpid());
		return pfile_open();
	}

	sleep(100);

	printf("make pipe\n");
	if (pipe(fds) == ERR) {
		return -3;
	}
	
	printf("pipe made fds = [%i, %i]\n", fds[0], fds[1]);


	printf("ppipe0\n");
	f = fork(FORK_sngroup);
	if (f < 0) {
		return -4;
	} else if (!f) {
		printf("forked to %i\n", getpid());
		close(fds[1]);
		return ppipe0(fds[0]);
	}
	
	close(fds[0]);
	
	sleep(100);

	printf("ppipe1\n");
	f = fork(FORK_sngroup);
	if (f < 0) {
		return -5;
	} else if (!f) {
		printf("forked to %i\n", getpid());
		return ppipe1(fds[1]);
	}

	printf("close\n");
		
	close(fds[1]);

	printf("pipe test done\n");
	
	sleep(100);

	printf("mmc\n");
	f = fork(FORK_sngroup);
	if (f < 0) {
		return -6;
	} else if (!f) {
		printf("forked to %i\n", getpid());
		return mmc();
	}

	sleep(100);
	return 0;
}
