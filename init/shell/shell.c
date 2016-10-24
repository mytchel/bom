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

static int funcexit(int argc, char **argv);
static int funccd(int argc, char **argv);
static int funcpwd(int argc, char **argv);

struct func funcs[] = {
  { "exit",      &funcexit },
  { "cd",        &funccd },
  { "pwd",       &funcpwd },
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
  int argc;

  argv[0] = strsection(line);
  if (argv[0] == nil) {
    return;
  } else {
    argc = 1;
  }

  while ((argv[argc] = strsection(nil)) != nil) {
    argc++;
  }

  if (runcmd(argc, argv, &ret)) {
    return;
  } else if (runfunc(argc, argv, &ret)) {
    return;
  } else {
    printf("%s: command not found\n", argv[0]);
  }
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
