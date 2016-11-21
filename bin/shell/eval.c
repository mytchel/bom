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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <fs.h>

#include "shell.h"

struct token *token;

struct exprbuilder {
  size_t bp;
  struct token * (*nud)(struct token *t);
  struct token * (*led)(struct token *t, struct token *left);
};

struct symbol {
  char *name;
  struct token *token;

  struct symbol *next;
};

static int
docmd(void);

static struct exprbuilder ops[];
static int argc = 0, in = STDIN, out = STDOUT;
static char *argv[MAXARGS];

static struct token *
rawnud(struct token *t)
{
  return t;
}

static struct token *
assignnud(struct token *t)
{
  printf("expected name before =\n");
  return nil;
}

static struct token *
assignled(struct token *t, struct token *left)
{
  struct token *n;
  
  if (left->type != TOKEN_NAME) {
    printf("expected name before =\n");
    return nil;
  }

  n = expression(0);

  return n;
}

static struct token *
substnud(struct token *t)
{
  return nil;
}

static struct token *
substled(struct token *t, struct token *left)
{
  printf("unexpected use of $\n");
  return nil;
}

static struct token *
plusnud(struct token *t)
{
  return expression(ops[TOKEN_PLUS].bp);
}

static struct token *
plusled(struct token *t, struct token *left)
{
  struct token *r;

  if (left->type != TOKEN_NUMBER) {
    printf("expected number before +\n");
    return nil;
  }
  
  r = expression(ops[TOKEN_PLUS].bp);
  if (r == nil) {
    printf("expected token\n");
    return nil;
  } else if (r->type != TOKEN_NUMBER) {
    printf("expected number after +\n");
    return nil;
  }

  t->type = TOKEN_NUMBER;
  t->val.num = left->val.num + r->val.num;
  return t;
}

static struct token *
minusnud(struct token *t)
{
  struct token *r;

  r = expression(ops[TOKEN_MINUS].bp);
  if (r->type != TOKEN_NUMBER) {
    printf("expected number after -\n");
    return nil;
  }

  r->val.num = -r->val.num;
  return r;
}


static struct token *
minusled(struct token *t, struct token *left)
{
  struct token *r;

  if (left->type != TOKEN_NUMBER) {
    printf("expected number before -\n");
    return nil;
  }
  
  r = expression(ops[TOKEN_MINUS].bp);
  if (r == nil) {
    printf("expected token\n");
    return nil;
  } else if (r->type != TOKEN_NUMBER) {
    printf("expected number after -\n");
    return nil;
  }

  t->type = TOKEN_NUMBER;
  t->val.num = left->val.num - r->val.num;
  return t;
}

static struct token *
endnud(struct token *t)
{
  int pid;
  
  pid = docmd();

  if (pid > 0) {
    while (wait(&ret) != pid)
      ;
  } else {
    ret = pid;
  }
  
  return nil;
}

static struct token *
pipenud(struct token *t)
{
  int p[2];

  if (pipe(p) != OK) {
    printf("pipe create failed.\n");
    return nil;
  }

  out = p[1];

  if (docmd() < 0) {
    printf("fork error.\n");
  }

  in = p[0];

  return nil;
}

static struct token *
bgnud(struct token *t)
{
  if (docmd() < 0) {
    printf("fork error.\n");
  }

  return nil;
}

static struct token *
andnud(struct token *t)
{
  int pid;
  
  pid = docmd();

  if (pid > 0) {
    while (wait(&ret) != pid)
      ;

    if (ret != OK) {
      /* no need to do any more */
      token = nil;
    }
  } else {
    printf("fork error.\n");
    ret = pid;
  }
  
  return nil;
}

static struct token *
ornud(struct token *t)
{
  int pid;
  
  pid = docmd();

  if (pid > 0) {
    while (wait(&ret) != pid)
      ;

    if (ret == OK) {
      /* no need to do any more */
      token = nil;
    }
  } else {
    printf("fork error.\n");
    ret = pid;
  }
  
  return nil;
}

static struct token *
atnud(struct token *t)
{
  return nil;
}

static struct token *
outnud(struct token *t)
{
  struct token *n;
  
  n = token;
  advance();

  if (n->type != TOKEN_NAME) {
    printf("expected file name after >\n");
    return nil;
  }

  out = open(n->val.str, O_WRONLY|O_TRUNC|O_CREATE,
	     ATTR_wr|ATTR_rd);

  if (out < 0) {
    printf("error opening %s : %i\n", n->val.str, out);
  }
  
  return nil;
}

static struct token *
appendnud(struct token *t)
{
  struct token *n;
  
  n = token;
  advance();

  if (n->type != TOKEN_NAME) {
    printf("expected file name after >\n");
    return nil;
  }

  out = open(n->val.str, O_WRONLY|O_APPEND|O_CREATE,
	     ATTR_wr|ATTR_rd);

  if (out < 0) {
    printf("error opening %s : %i\n", n->val.str, out);
  }
  
  return nil;
}

static struct token *
innud(struct token *t)
{
  struct token *n;
  
  n = token;
  advance();

  if (n->type != TOKEN_NAME) {
    printf("expected file name after >\n");
    return nil;
  }

  in = open(n->val.str, O_RDONLY|O_CREATE,
	     ATTR_wr|ATTR_rd);

  if (in < 0) {
    printf("error opening %s : %i\n", n->val.str, in);
  }
  
  return nil;
}

static struct exprbuilder ops[] = {
  [TOKEN_NONE]         = {  0,   nil,                nil },

  [TOKEN_NAME]         = {  0,   rawnud,             nil },
  [TOKEN_NUMBER]       = {  0,   rawnud,             nil },
  [TOKEN_LIST]         = {  0,   rawnud,             nil },

  [TOKEN_PARAN_LEFT]   = { 80,   nil,                nil },
  [TOKEN_PARAN_RIGHT]  = {  0,   nil,                nil },
  [TOKEN_BRACE_LEFT]   = { 80,   nil,                nil },
  [TOKEN_BRACE_RIGHT]  = {  0,   nil,                nil },
  [TOKEN_BRACKET_LEFT] = { 80,   nil,                nil },
  [TOKEN_BRACKET_RIGHT]= {  0,   nil,                nil },
  [TOKEN_ASSIGN]       = {  6,   assignnud,          assignled},
  [TOKEN_EQUAL]        = {  5,   nil,                nil },
  [TOKEN_PLUS]         = { 50,   plusnud,            plusled },
  [TOKEN_MINUS]        = { 50,   minusnud,           minusled },
  [TOKEN_CARET]        = { 20,   nil,                nil },
  [TOKEN_SUBST]        = {  0,   substnud,           substled},
  [TOKEN_STAR]         = { 10,   nil,                nil },
  [TOKEN_AND]          = {  0,   andnud,             nil },
  [TOKEN_BG]           = {  0,   bgnud,              nil },
  [TOKEN_PIPE]         = {  0,   pipenud,            nil },
  [TOKEN_OR]           = {  0,   ornud,              nil },
  [TOKEN_AT]           = {  0,   atnud,              nil },
  [TOKEN_OUT]          = { 10,   outnud,             nil },
  [TOKEN_APPEND]       = { 10,   appendnud,          nil },
  [TOKEN_IN]           = { 10,   innud,              nil },
  [TOKEN_END]          = {  0,   endnud,             nil },
};

void
advance(void)
{
  token = token->next;
}

struct token *
expression(int bp)
{
  struct token *t, *left;

  t = token;
  advance();
  left = ops[t->type].nud(t);

  while (left != nil && bp < ops[token->type].bp) {
    t = token;
    advance();
    left = ops[t->type].led(t, left);
  }

  return left;
}

static int
docmd(void)
{
  char tmp[LINEMAX];
  int pid, r;

  if (argc > 0) {
    pid = fork(FORK_sngroup);
  } else {
    pid = 0;
  }
  
  if (argc == 0 || pid != 0) {
    for (r = 0; r < argc; r++) {
      free(argv[r]);
    }

    argc = 0;
 
    if (in != STDIN) {
      close(in);
      in = STDIN;
    }

    if (out != STDOUT) {
      close(out);
      out = STDOUT;
    }

    return pid;
  }

  if (in < 0) {
    exit(in);
    return 0;
  } else if (out < 0) {
    exit(out);
    return 0;
  }
  
  if (in != STDIN) {
    dup2(in, STDIN);
    close(in);
  }

  if (out != STDOUT) {
    dup2(out, STDOUT);
    close(out);
  }

  if (argv[0][0] == '/' || argv[0][0] == '.') {
    r = exec(argv[0], argc, argv);
  } else {
    snprintf(tmp, LINEMAX, "/bin/%s", argv[0]);
    r = exec(tmp, argc, argv);
  }

  printf("%s : failed to run, %i\n", argv[0], r);
  exit(r);

  return 0;
}

void
eval(void)
{
  struct token *t;
  
  while (token != nil) {
    if (token->type == TOKEN_NAME) {
      t = token;
      advance();
    } else {
      t = expression(0);
    }

    if (t == nil) {
      continue;
    }
 
    argv[argc] = malloc(sizeof(char) * LINEMAX);
    if (argv[argc] == nil) {
      printf("malloc failed.\n");
      exit(ENOMEM);
    }

    if (t->type == TOKEN_NAME) {
      snprintf(argv[argc++], LINEMAX, "%s", t->val.str);

    } else if (t->type == TOKEN_NUMBER) {

      snprintf(argv[argc++], LINEMAX, "%i", t->val.num);

    } else if (t->type == TOKEN_LIST) {

    } else {
      printf("expected name or number\n");
    }
  }
}
