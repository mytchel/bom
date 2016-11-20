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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "shell.h"

struct func {
  char *name;
  int (*func)(struct atom *cmd, struct atom *rest, struct atom *delim);
};


static int funcexit(struct atom *, struct atom *, struct atom *);
static int funccd(struct atom *, struct atom *, struct atom *);
static int funcif(struct atom *, struct atom *, struct atom *);
static int funcfor(struct atom *, struct atom *, struct atom *);

static struct func funcs[] = {
  { "exit",      &funcexit },
  { "cd",        &funccd },
  { "if",        &funcif },
  { "for",       &funcfor },
};

int
funcexit(struct atom *cmd, struct atom *rest, struct atom *delim)
{
  struct atom *ap, *t;
  int code;

  ap = cmd;
  while (rest != delim) {
    while (ap->next != nil) {
      ap = ap->next;
    }

    t = flatten(rest);
    if (t == nil) {
      return ERR;
    }

    rest = rest->next;
    ap->next = t;
 }

  if (cmd->next == nil) {
    exit(OK);
  } else if (cmd->next->next != nil) {
    printf("usage: %s [exitcode]\n", cmd->a.str);
    atomfreel(cmd);
  } else {
    code = strtol(cmd->next->a.str, nil, 10);
    exit(code);
  }

  return ERR;
}

int
funccd(struct atom *cmd, struct atom *rest, struct atom *delim)
{
  struct atom *ap, *t;

  ap = cmd;
  while (rest != delim) {
    while (ap->next != nil) {
      ap = ap->next;
    }

    t = flatten(rest);
    if (t == nil) {
      return ERR;
    }

    rest = rest->next;
    ap->next = t;
 }

  if (cmd->next == nil || cmd->next->next != nil) {
    printf("usage: %s path/to/dir\n", cmd->a.str);
    return ERR;
  } else {
    return chdir(cmd->next->a.str);
  }
}

int
funcif(struct atom *cmd, struct atom *rest, struct atom *delim)
{
  

  return ERR;
}

int
funcfor(struct atom *cmd, struct atom *rest, struct atom *delim)
{
  return ERR;
}

bool
runfunc(struct atom *cmd, struct atom *rest, struct atom *delim)
{
  int i;

  for (i = 0; i < sizeof(funcs) / sizeof(funcs[0]); i++) {
    if (strcmp(funcs[i].name, cmd->a.str)) {
      ret = funcs[i].func(cmd, rest, delim);
      return true;
    }
  }

  return false;
}
