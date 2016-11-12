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

struct part {
  int argc;
  char **argv;
};

static int funcexit(int argc, char **argv);
static int funccd(int argc, char **argv);
static int funcret(int argc, char **argv);

struct func funcs[] = {
  { "exit",      &funcexit },
  { "cd",        &funccd },
  { "$?",        &funcret },
};

static int ret = 0;

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

  return 0;
}

int
funcret(int argc, char **argv)
{
  printf("%i\n", ret);

  return OK;
}

bool
runfunc(int argc, char *argv[], int *ret)
{
  int i;
  
  for (i = 0; i < sizeof(funcs) / sizeof(funcs[0]); i++) {
    if (strcmp(funcs[i].name, argv[0])) {
      *ret = funcs[i].func(argc, argv);
      return true;
    }
  }

  return false;
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

void
processsentence(char *line)
{
  char *argv[MAX_ARGS];
  int argc, pid, p;

  argv[0] = strsection(line);
  if (argv[0] == nil) {
    return;
  } else {
    argc = 1;
  }

  while ((argv[argc] = strsection(nil)) != nil) {
    argc++;
  }

  if (runfunc(argc, argv, &ret)) {
    return;
  }

  pid = fork(FORK_sngroup);
  if (pid == 0) {
    runcmd(argc, argv);
  } else {
    while ((p = wait(&ret)) != pid)
      ;
  }
}
