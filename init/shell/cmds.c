/*
 * Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <libc.h>
#include <mem.h>
#include <fs.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "shell.h"

int
mounttmp(char *path);

int
mountfat(char *device, char *dir);

static int cmdpwd(int argc, char **argv);
static int cmdls(int argc, char **argv);
static int cmdecho(int argc, char **argv);
static int cmdmkdir(int argc, char **argv);
static int cmdtouch(int argc, char **argv);
static int cmdrm(int argc, char **argv);
static int cmdcat(int argc, char **argv);
static int cmdcp(int argc, char **argv);
static int cmdmounttmp(int argc, char **argv);
static int cmdmountfat(int argc, char **argv);

struct func cmds[] = {
  { "pwd",       &cmdpwd },
  { "ls",        &cmdls },
  { "echo",      &cmdecho },
  { "mkdir",     &cmdmkdir },
  { "touch",     &cmdtouch },
  { "rm",        &cmdrm },
  { "cat",       &cmdcat },
  { "cp",        &cmdcp },
  { "mounttmp",  &cmdmounttmp },
  { "mountfat",  &cmdmountfat },
};

int
cmdpwd(int argc, char **argv)
{
  char pwd[NAMEMAX * 10] = ".";

  cleanpath(pwd, pwd + 1, sizeof(pwd) - 1);

  pwd[0] = '/';

  printf("%s\n", pwd);

  return OK;
}

int
cmdlsh(char *filename)
{
  uint8_t buf[NAMEMAX * 3], len;
  char path[NAMEMAX * 10];
  struct stat s;
  int r, fd, i;

  if (stat(filename, &s) != OK) {
    printf("Error statting '%s'\n", filename);
    return ERR;
  }

  if (!(s.attr & ATTR_dir)) {
    printf("%b %u %s\n", s.attr, s.size, filename);
    return OK;
  }

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    printf("Error opening '%s'\n", filename);
    return ERR;
  }

  i = 0;
  while ((r = read(fd, &buf[i], sizeof(buf) - i)) > 0) {
    r += i;
    i = 0;
    while (i < r) {
      len = buf[i];

      if (i + 1 + len > r) {
	/* copy remander of buf to start */
	memmove(buf, &buf[i], r - i);
	i = r - i;
	break;
      }

      i++;

      snprintf(path, sizeof(path), "%s/%s", filename, &buf[i]);
      if (stat(path, &s) != OK) {
	printf("stat error %s\n", path);
      } else {
	printf("%b %u %s\n", s.attr, s.size, &buf[i]);
      }

      i += len;
    }

    if (r == i) {
      i = 0;
    }
  }

  close(fd);

  return OK;
}

int
cmdls(int argc, char **argv)
{
  int i;

  if (argc == 1) {
    cmdlsh(".");
  } else if (argc == 2) {
    cmdlsh(argv[1]);
  } else {
    for (i = 1; i < argc; i++) {
      printf("%s:\n", argv[i]);
      cmdlsh(argv[i]);
    }
  }
  
  return OK;
}

int
cmdecho(int argc, char **argv)
{
  int i;

  for (i = 1; i < argc; i++) {
    if (i == argc - 1) {
      printf("%s", argv[i]);
    } else {
      printf("%s ", argv[i]);
    }
  }

  printf("\n");
  
  return OK;
}

int
cmdmkdir(int argc, char **argv)
{
  int i, fd;

  for (i = 1; i < argc; i++) {
    fd = open(argv[i], O_RDONLY|O_CREATE,
	      ATTR_rd|ATTR_wr|ATTR_dir);

    if (fd < 0) {
      printf("mkdir %s failed.\n", argv[i]);
      return fd;
    } else {
      close(fd);
    }
  }

  return OK;
}

int
cmdtouch(int argc, char **argv)
{
  int i, fd;

  for (i = 1; i < argc; i++) {
    fd = open(argv[i], O_RDONLY|O_CREATE,
	      ATTR_rd|ATTR_wr);

    if (fd < 0) {
      printf("touch %s failed.\n", argv[i]);
      return fd;
    } else {
      close(fd);
    }
  }

  return OK;
}

int
cmdrm(int argc, char **argv)
{
  int i, e;

  for (i = 1; i < argc; i++) {
    e = remove(argv[i]);
    if (e != OK) {
      printf("rm %s failed ERR %i\n", argv[i], e);
      return e;
    }
  }

  return OK;
}

int
cmdcat(int argc, char **argv)
{
  uint8_t buf[512];
  int i, fd, l;

  for (i = 1; i < argc; i++) {
    fd = open(argv[i], O_RDONLY);
    if (fd < 0) {
      printf("cat %s failed %i\n", argv[i], fd);
      return fd;
    } else {
      while ((l = read(fd, buf, sizeof(buf))) > 0) {
	write(stdout, buf, l);
      }

      if (l != EOF) {
	printf("read %s failed %i\n", argv[i], l);
      }

      close(fd);
    }
  }

  return OK;
}

int
cmdcp(int argc, char **argv)
{
  uint8_t buf[512];
  int r, in, out;

  in = open(argv[1], O_RDONLY);
  if (in < 0) {
    printf("cp failed to open %s %i\n", argv[1], in);
    return in;
  }

  out = open(argv[2], O_WRONLY|O_CREATE, ATTR_rd|ATTR_wr);
  if (out < 0) {
    printf("cp failed to open %s %i\n", argv[2], out);
    return out;
  }

  printf("in and out open, copy\n");
				      
  while ((r = read(in, buf, sizeof(buf))) > 0) {
    if ((r = write(out, buf, r)) < 0) {
      printf("cp failed to write %s %i\n", argv[2], r);
      return r;
    }
  }

  if (r != EOF) {
    printf("cp failed to read %s %i\n", argv[1], r);
  } 

  close(in);
  close(out);

  return OK;
}

int
cmdmounttmp(int argc, char **argv)
{
  if (argc != 2) {
    printf("usage: %s dir\n", argv[0]);
    return ERR;
  }

  return mounttmp(argv[1]);
}

int
cmdmountfat(int argc, char **argv)
{
  if (argc != 3) {
    printf("usage: %s blockdevice dir\n", argv[0]);
    return ERR;
  }

  return mountfat(argv[1], argv[2]);
}

bool
runcmd(int argc, char *argv[], int *ret)
{
  int i, r;
  
  for (i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
    if (strcmp(cmds[i].name, argv[0])) {
      r = fork(FORK_sngroup);
      if (r == 0) {
	r = cmds[i].func(argc, argv);
	exit(r);
      } else {
	wait(ret);
	return true;
      }
    }
  }

  return false;
}

