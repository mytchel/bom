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
#include <fs.h>
#include <stdarg.h>

int
ppipe0(int fd);

int
ppipe1(int fd);

int
pfile_open(void);

int
commount(char *path);

int
tmpmount(char *path);

int
mmc(void);

extern struct fsmount fsmount;
int stdin, stdout, stderr;

int
main(void)
{
  int f, fds[2];

  f = open("/dev", O_WRONLY|O_CREATE, ATTR_wr|ATTR_rd|ATTR_dir);
  if (f < 0) {
    return -1;
  }

  f = commount("/dev/com");
  if (f < 0) {
    return -1;
  }

  stdin = open("/dev/com", O_RDONLY);
  stdout = open("/dev/com", O_WRONLY);
  stderr = open("/dev/com", O_WRONLY);

  if (stdin < 0) return -2;
  if (stdout < 0) return -3;
  if (stderr < 0) return -3;

  printf("/dev/com mounted pid %i\n", f);

  f = tmpmount("/tmp");
  if (f < 0) {
    return -1;
  }

  printf("/tmp mounted on pid %i\n", f);
  
   f = fork(FORK_sngroup);
  if (f < 0) {
    return -6;
  } else if (!f) {
    printf("mmc on %i\n", getpid());
    return mmc();
  }

  f = fork(FORK_sngroup);
  if (f < 0) {
    return -2;
  } else if (!f) {
    printf("pfile_open on %i\n", getpid());
    return pfile_open();
  }

  if (pipe(fds) == ERR) {
    return -3;
  }
	
  f = fork(FORK_sngroup);
  if (f < 0) {
    return -4;
  } else if (!f) {
    printf("ppipe0 on %i\n", getpid());
    close(fds[1]);
    return ppipe0(fds[0]);
  }
	
  close(fds[0]);
	
  f = fork(FORK_sngroup);
  if (f < 0) {
    return -5;
  } else if (!f) {
    printf("ppipe1 on %i\n", getpid());
    return ppipe1(fds[1]);
  }

  close(fds[1]);

  return 0;
}
