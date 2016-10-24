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

#define LINE_MAX 512
#define MAX_ARGS 512

struct func {
  char *name;
  int (*func)(int argc, char **argv);
};

int
mounttmp(char *path);

int
mountfat(char *device, char *dir);

static int funcexit(int argc, char **argv);
static int funccd(int argc, char **argv);
static int funcpwd(int argc, char **argv);

static int cmdls(int argc, char **argv);
static int cmdecho(int argc, char **argv);
static int cmdmkdir(int argc, char **argv);
static int cmdtouch(int argc, char **argv);
static int cmdrm(int argc, char **argv);
static int cmdcat(int argc, char **argv);
static int cmdmounttmp(int argc, char **argv);
static int cmdmountfat(int argc, char **argv);

struct func funcs[] = {
  { "exit",      &funcexit },
  { "cd",        &funccd },
  { "pwd",       &funcpwd },
};

struct func cmds[] = {
  { "ls",        &cmdls },
  { "echo",      &cmdecho },
  { "mkdir",     &cmdmkdir },
  { "touch",     &cmdtouch },
  { "rm",        &cmdrm },
  { "cat",       &cmdcat },
  { "mounttmp",  &cmdmounttmp },
  { "mountfat",  &cmdmountfat },
};

static int ret = 0;
static char pwd[NAMEMAX * 10] = "/";

int
funcexit(int argc, char **argv)
{
  int code = 1;
  
  if (argc == 2) {
    code = strtol(argv[1], nil, 10);
  }

  exit(code);

  return ERR;
}

int
funccd(int argc, char **argv)
{
  char *dir;
  
  if (argc > 2) {
    printf("usage: %s dir\n", argv[0]);
    return ERR;
  } else if (argc == 1) {
    dir = "/";
  } else {
    dir = argv[1];
  }

  if (chdir(dir) != OK) {
    printf("Failed to change to '%s'\n", dir);
    return ERR;
  }

  memmove(pwd, ".", 2);
  cleanpath(pwd, pwd+1, sizeof(pwd));
  pwd[0] = '/';
  
  return 0;
}

int
funcpwd(int argc, char **argv)
{
  printf("%s\n", pwd);
  return OK;
}

int
cmdlsh(char *filename)
{
  uint8_t buf[NAMEMAX + 1], len;
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

  if (chdir(filename) != OK) {
    printf("Failed to descend into '%s'\n", filename);
    return ERR;
  }
  
  fd = open(".", O_RDONLY);
  if (fd < 0) {
    printf("Error opening '%s'\n", filename);
    return ERR;
  }

  i = 0;
  while ((r = read(fd, &buf[i], sizeof(buf) - i)) > 0) {
    while (i < r) {
      len = buf[i];

      if (i + len >= r) {
	/* copy remander of buf to start */
	memmove(buf, &buf[i], r - i);
	i = r - i;
	break;
      }

      i++;

      if (stat((const char *) &buf[i], &s) != OK) {
	printf("stat error %s\n", buf);
      } else {
	printf("%b %u %s\n", s.attr, s.size, &buf[i]);
      }

      i += len;
    }

    if (i == r) {
      i = 0;
    }
  }

  close(fd);

  chdir(pwd);

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
      printf("cat %s failed ERR %i\n", argv[i], fd);
      return fd;
    } else {
      while ((l = read(fd, buf, sizeof(buf))) > 0) {
	write(stdout, buf, l);
      }
    }
  }

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
   
static int
readline(uint8_t *data, size_t max)
{
  size_t i;
  char c;

  i = 0;
  while (i < max) {
    if (read(stdin, &c, sizeof(char)) < 0) {
      return -1;
    } else if (c == '\n') {
      data[i] = '\0';
      return i;
    } else {
      data[i++] = c;
    }
  }

  data[i-1] = 0;
  return i;
}

static void
shiftstringleft(char *str)
{
  while (*(str++)) {
    *(str-1) = *str;
  }
}

/* Like strtok but allows for quotes and escapes. */
static char *
strsection(char *nstr)
{
  static char *str = nil;
  char *c;

  if (nstr != nil) {
    str = nstr;
  } else if (str == nil) {
    return nil;
  }

  while (*str != 0 && isspace(*str))
    str++;

  if (*str == 0) {
    return nil;
  }

  c = str;
  while (*c != 0) {
    if (isspace(*c)) {
      break;
    } else if (*c == '\\') {
      shiftstringleft(c);
      c++;
    } else if (*c == '\'') {
      shiftstringleft(c);

      while (*c != 0 && *c != '\'')
	c++;

      shiftstringleft(c);
    }

    c++;
  }

  nstr = str;
  if (*c == 0) {
    str = nil;
  } else {
    *c = 0;
    str = c + 1;
  }

  return nstr;
}

static void
processline(char *line)
{
  char *argv[MAX_ARGS];
  int i, f, argc;

  argv[0] = strsection(line);
  if (argv[0] == nil) {
    return;
  } else {
    argc = 1;
  }

  while ((argv[argc] = strsection(nil)) != nil) {
    argc++;
  }

  for (i = 0; i < sizeof(funcs) / sizeof(funcs[0]); i++) {
    if (strcmp(funcs[i].name, argv[0])) {
      ret = funcs[i].func(argc, argv);
      return;
    }
  }

  for (i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
    if (strcmp(cmds[i].name, argv[0])) {
      f = fork(FORK_sngroup);
      if (f == 0) {
	ret = cmds[i].func(argc, argv);
	exit(ret);
      } else {
	i = wait(&ret);
	return;
      }
    }
  }


  printf("%s: command not found\n", argv[0]);
}

int
shell(void)
{
  uint8_t line[LINE_MAX];

  while (true) {
    printf("%% ");
    readline(line, LINE_MAX);
    processline((char *) line);
  }

  /* Never reached */
  exit(OK);
}
