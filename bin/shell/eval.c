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

static struct command *cmd = nil;

static struct command *
commandnew(void)
{
  struct command *c;

  c = malloc(sizeof(struct command));
  if (c == nil) {
    printf("malloc failed.\n");
    exit(ENOMEM);
  }
  
  c->in = nil;
  c->inmode = 0;
  c->out = nil;
  c->outmode = 0;
  c->next = nil;
  c->parent = nil;
  c->args = nil;

  return c;
}

static void
commandpop(void)
{
  cmd = cmd->parent;
  if (cmd == nil) {
    cmd = commandnew();
  }
}

static struct command *
commandpush(void)
{
  struct command *c;

  c = commandnew();
  c->parent = cmd;
  cmd = c;

  return c;
}

static struct token *
buildlistled(struct token *self, struct token *left)
{
  struct list *l;
  struct token *n;
  
  self->next = nil;
  if (left->type == TOKEN_LIST) {
    l = (struct list *) left->aux;

    if (l->tail == nil) {
      l->head = l->tail = self;
    } else {
      l->tail->next = self;
      l->tail = self;
    }
    
    return left;
  } else {
    l = malloc(sizeof(struct list));
    if (l == nil) {
      printf("malloc failed.\n");
      exit(ENOMEM);
    }

    l->head = left;
    left->next = self;
    l->tail = self;

    n = tokennew(TOKEN_LIST);
    n->aux = l;
    return n;
  }
}

static void
namefree(struct token *self)
{
  if (self->aux != nil) {
    free(self->aux);
  }
  
  free(self);
}

static void
rawfree(struct token *self)
{
  free(self);
}

static struct token *
rawnud(struct token *self)
{
  return self;
}

static void
listfree(struct token *self)
{
  struct token *n, *t;
  struct list *l;

  l = (struct list *) self->aux;

  t = l->head;
  while (t != nil) {
    n = t->next;
    types[t->type].free(t);
    t = n;
  }

  free(self);
}

static struct token *
endnud(struct token *self)
{
  types[self->type].free(self);
  return nil;
}

static struct token *
endled(struct token *self, struct token *left)
{
  types[left->type].free(left);
  types[self->type].free(self);
  return nil;
}

static struct token *
bracenud(struct token *self)
{
  struct token *t, *b;
  token_t type;
  
  types[self->type].free(self);

  t = command(0);

  b = token;
  advance();

  type = b->type;
  types[type].free(b);
  
  if (type != TOKEN_BRACE_RIGHT) {
    types[t->type].free(t);
    return nil;
  } else {
    return t;
  }
}

static struct token *
braceled(struct token *self, struct token *left)
{
  printf("unexpected statment before {\n");
  return nil;
}

static void
bracketleftfree(struct token *self)
{
  free(self);
}

static void
equalfree(struct token *self)
{
  free(self);
}

static void
assignfree(struct token *self)
{
  free(self);
}

static struct token *
assignnud(struct token *self)
{
  return nil;
}

static struct token *
assignled(struct token *self, struct token *left)
{
  return nil;
}

static void
substfree(struct token *self)
{
  free(self);
}

static struct token *
substnud(struct token *self)
{
  return nil;
}

static struct token *
substled(struct token *self, struct token *left)
{
  return nil;
}

static struct token *
plusnud(struct token *self)
{
  struct token *n;

  n = expression(types[self->type].bp);

  types[self->type].free(self);
  if (n->type != TOKEN_NUMBER) {
    printf("expected number after +\n");
    types[n->type].free(n);
    return nil;
  } else {
    return n;
  }
}

static struct token *
plusled(struct token *self, struct token *left)
{
  struct token *n;

  if (left->type != TOKEN_NUMBER) {
    printf("expected number before +\n");
    return nil;
  }

  n = expression(types[self->type].bp);

  if (n->type != TOKEN_NUMBER) {
    printf("expected number after +\n");
    return nil;
  }

  self->type = TOKEN_NUMBER;
  self->aux = (void *) ((int) left->aux + (int) n->aux);

  types[left->type].free(left);
  types[n->type].free(n);

  return self;
}

static struct token *
minusnud(struct token *self)
{
  struct token *n;

  n = expression(types[self->type].bp);

  types[self->type].free(self);
  if (n->type != TOKEN_NUMBER) {
    printf("expected number after -\n");
    types[n->type].free(n);
    return nil;
  } else {
    n->aux = (void *) (- (int) n->aux);
    return n;
  }
}

static struct token *
minusled(struct token *self, struct token *left)
{
  struct token *n;

  if (left->type != TOKEN_NUMBER) {
    printf("expected number before -\n");
    return nil;
  }

  n = expression(types[self->type].bp);

  if (n->type != TOKEN_NUMBER) {
    printf("expected number after -\n");
    return nil;
  }

  self->type = TOKEN_NUMBER;
  self->aux = (void *) ((int) left->aux - (int) n->aux);

  types[left->type].free(left);
  types[n->type].free(n);

  return self;
}

static struct token *
outnud(struct token *self)
{
  printf("expected expression before >\n");
  return nil;
}

static struct token *
outled(struct token *self, struct token *left)
{
  struct token *t;

  types[self->type].free(self);

  t = token;
  advance();
  if (t->type != TOKEN_NAME) {
    printf("expected file after >\n");
    types[t->type].free(t);
    return nil;
  }

  if (cmd->out != nil) {
    free(cmd->out);
  }
  
  cmd->out = t->aux;
  cmd->outmode = O_TRUNC;

  t->aux = nil;
  types[t->type].free(t);

  return left;
}

static struct token *
appendnud(struct token *self)
{
  printf("expected expression before >>\n");
  return nil;
}

static struct token *
appendled(struct token *self, struct token *left)
{
  struct token *t;

  types[self->type].free(self);

  t = token;
  advance();
  if (t->type != TOKEN_NAME) {
    printf("expected file after >>\n");
    types[t->type].free(t);
    return nil;
  }

  if (cmd->out != nil) {
    free(cmd->out);
  }
  
  cmd->out = t->aux;
  cmd->outmode = O_APPEND;

  t->aux = nil;
  types[t->type].free(t);

  return left;
}

static struct token *
innud(struct token *self)
{
  printf("expected expression before <\n");
  return nil;
}

static struct token *
inled(struct token *self, struct token *left)
{
  struct token *t;

  types[self->type].free(self);

  t = token;
  advance();
  if (t->type != TOKEN_NAME) {
    printf("expected file after <\n");
    types[t->type].free(t);
    return nil;
  }

  if (cmd->in != nil) {
    free(cmd->in);
  }
  
  cmd->in = t->aux;

  t->aux = nil;
  types[t->type].free(t);

  return left;
}

static void
cdfree(struct token *self)
{
  free(self);
}

static struct token *
cdnud(struct token *self)
{
  return nil;
}

static void
iffree(struct token *self)
{
  free(self);
}

static struct token *
ifnud(struct token *self)
{
  return nil;
}

static void
forfree(struct token *self)
{
   free(self); 
}

static struct token *
fornud(struct token *self)
{
  return nil;
}

static void
commandfree(struct token *self)
{
  struct command *c;

  c = ((struct command *) self->aux);

  if (c->args != nil) {
    types[c->args->type].free(c->args);
  }

  if (c->next != nil) {
    types[c->next->type].free(c->next);
  }

  if (c->in != nil) {
    free(c->in);
  }

  if (c->out != nil) {
    free(c->out);
  }

  free(c);
  free(self);
}

static struct token *
commandled(struct token *self, struct token *left)
{
  struct command *c;
  
  c = cmd;

  c->args = left;
  c->type = self->type;
  c->next = nil;

  self->type = TOKEN_COMMAND;
  self->aux = c;
  self->next = nil;

  c->next = command(types[c->type].bp);

  switch (c->type) {
  case TOKEN_BG:
  case TOKEN_SEMI:
    break;

  case TOKEN_AND:
  case TOKEN_OR:
  case TOKEN_PIPE:
    if (c->next == nil) {
      printf("expected statement after %s.\n", types[c->type].str);
      types[self->type].free(self);
      return nil;
    }

    break;

  default:
    printf("This should never happen\n");
    types[self->type].free(self);
    return nil;
  }

  commandpop();
  
  return self;
}

struct token *
expression(int bp)
{
  struct token *t, *left;

  t = token;
  advance();
  left = types[t->type].nud(t);

  while (left != nil && bp < types[token->type].bp) {
    t = token;
    advance();
    left = types[t->type].led(t, left);
  }

  return left;
}

struct token *
command(int bp)
{
  struct token *t, *w;
  struct command *p;

  p = cmd;
  commandpush();

  t = expression(bp);

  if (t == nil) {
    free(cmd);
    cmd = p;
    return nil;

  } else if (t->type == TOKEN_COMMAND) {
    return t;

  } else {
    /* Need to wrap it in a command */
    w = tokennew(TOKEN_COMMAND);
    w->aux = cmd;
    cmd->type = TOKEN_SEMI;
    cmd->args = t;

    commandpop();

    return w;
  }
}

static int
convertargs(struct token *t, int argc, char *argv[])
{
  struct list *l;
  
  while (t != nil) {
    if (t->type == TOKEN_NAME) {
      argv[argc++] = t->aux;
      t->aux = nil;
    } else if (t->type == TOKEN_LIST) {
      l = (struct list *) t->aux;
      argc = convertargs(l->head, argc, argv);
    }

    t = t->next;
  }

  return argc;
}

static int
docmd(struct command *c, int infd, int outfd)
{
  char *argv[MAXARGS], tmp[LINEMAX];
  int pid, argc, r;
  
  pid = fork(FORK_sngroup);
  
  if (pid != 0) {
    return pid;
  }

  if (c->in != nil) {
    infd = open(c->in, c->inmode|O_RDONLY);
    if (infd < 0) {
      printf("failed to open %s: %i\n", c->in, infd);
      exit(infd);
    }
  }

  if (c->out != nil) {
    outfd = open(c->out, c->outmode|O_WRONLY|O_CREATE, ATTR_rd|ATTR_wr);
    if (outfd < 0) {
      printf("failed to open %s: %i\n", c->out, outfd);
      exit(outfd);
    }
  }

  if (infd != STDIN) {
    dup2(outfd, STDIN);
    close(outfd);
  }

  if (outfd != STDOUT) {
    dup2(outfd, STDOUT);
    close(outfd);
  }

  if (c->args == nil) {
    exit(ERR);

  } else if (c->args->type == TOKEN_NUMBER) {
    printf("%i\n", c->args->aux);
    exit(OK);

  } else if (c->args->type == TOKEN_LIST || c->args->type == TOKEN_NAME) {
    argc = convertargs(c->args, 0, argv);

    if (argv[0][0] == '/' || argv[0][0] == '.') {
      r = exec(argv[0], argc, argv);
    } else {
      snprintf(tmp, LINEMAX, "/bin/%s", argv[0]);
      r = exec(tmp, argc, argv);
    }

    if (r == ENOFILE) {
      printf("%s : command not found\n", argv[0]);
    } else {
      printf("%s : failed to run, %i\n", argv[0], r);
    }
  
    exit(r);
 
  } else {
    r = types[c->args->type].eval(c->args);
    exit(r);
  }
  
  return 0;
}

static int
commandevalh(struct token *self, int in, int out)
{
  struct command *c;
  int pid, r, p[2];

  c = self->aux;
  if (c == nil) {
    printf("command aux is nil!\n");
    return ERR;
  }

  switch (c->type) {
  case TOKEN_PIPE:
    if (pipe(p) != OK) {
      printf("pipe error!\n");
      return ERR;
    }

    pid = docmd(c, in, p[1]);
    if (pid < 0) {
      printf("fork error!\n");
      return pid;
    }

    close(p[1]);
    r = commandevalh(c->next, p[0], out);
    close(p[0]);

    return r;

  case TOKEN_AND:
    pid = docmd(c, in, out);
    if (pid < 0) {
      printf("fork error!\n");
      return pid;
    }

    while (wait(&r) != pid)
      ;

    if (r == OK) {
      return commandevalh(c->next, in, out);
    } else {
      return r;
    }

  case TOKEN_OR:
    pid = docmd(c, in, out);
    if (pid < 0) {
      printf("fork error!\n");
      return pid;
    }

    while (wait(&r) != pid)
      ;

    if (r == OK) {
      return OK;
    } else {
      return commandevalh(c->next, in, out);
    }

  case TOKEN_BG:
    pid = docmd(c, in, out);
    if (pid < 0) {
      printf("fork error!\n");
      return pid;
    }
    
    if (c->next == nil) {
      return OK;
    } else {
      return commandevalh(c->next, in, out);
    }
    
  case TOKEN_SEMI:
    pid = docmd(c, in, out);
    if (pid < 0) {
      printf("fork error!\n");
      return pid;
    }
    
    while (wait(&r) != pid)
      ;

    if (c->next == nil) {
      return r;
    } else {
      return commandevalh(c->next, in, out);
    }
 
  default:
    printf("cmd type unknown, this should never happen!\n");
    return ERR;
  }
}

static int
commandeval(struct token *self)
{
  return commandevalh(self, STDIN, STDOUT);
}

struct tokentype types[TOKEN_TYPES] = {
  [TOKEN_PARAN_LEFT]   = { true,  1,   "(", 80, rawfree, nil, nil, nil },
  [TOKEN_PARAN_RIGHT]  = { true,  1,   ")",  0, rawfree, nil, nil, nil },

  [TOKEN_BRACE_LEFT]   = { true,  1,   "{", 80, rawfree, bracenud, braceled, nil },
  [TOKEN_BRACE_RIGHT]  = { true,  1,   "}",  0, rawfree, nil, nil, nil },

  [TOKEN_BRACKET_LEFT] = { true,  1,   "[", 80, bracketleftfree, nil, nil, nil },
  [TOKEN_BRACKET_RIGHT]= { true,  1,   "]",  0, rawfree, nil, nil, nil },

  [TOKEN_EQUAL]        = { true,  2,  "==", 10, equalfree, nil, nil, nil },
  [TOKEN_ASSIGN]       = { true,  1,   "=",  9, assignfree, assignnud, assignled, nil },
  [TOKEN_PLUS]         = { true,  1,   "+", 50, rawfree, plusnud, plusled, nil },
  [TOKEN_MINUS]        = { true,  1,   "-", 50, rawfree, minusnud, minusled, nil },
  [TOKEN_SUBST]        = { true,  1,   "$",  0, substfree, substnud, substled, nil },

  [TOKEN_APPEND]       = { true,  2,  ">>", 10, rawfree, appendnud, appendled, nil },
  [TOKEN_OUT]          = { true,  1,   ">", 10, rawfree, outnud, outled, nil },
  [TOKEN_IN]           = { true,  1,   "<", 10, rawfree, innud, inled, nil },

  [TOKEN_CD]           = { false, 2,  "cd", 10, cdfree, cdnud, nil, nil },
  [TOKEN_IF]           = { false, 2,  "if", 10, iffree, ifnud, nil, nil },
  [TOKEN_FOR]          = { false, 3, "for", 10, forfree, fornud, nil, nil },

  [TOKEN_AND]          = { true,  2,  "&&",  2, rawfree, rawnud, commandled, nil },
  [TOKEN_OR]           = { true,  2,  "||",  2, rawfree, rawnud, commandled, nil },
  [TOKEN_PIPE]         = { true,  1,   "|",  2, rawfree, rawnud, commandled, nil },
  [TOKEN_BG]           = { true,  1,   "&",  1, rawfree, rawnud, commandled, nil },
  [TOKEN_SEMI]         = { true,  1,   ";",  1, rawfree, rawnud, commandled, nil },

  [TOKEN_NAME]         = { false, 0,   nil,  5, namefree, rawnud, buildlistled, nil },
  [TOKEN_NUMBER]       = { false, 0,   nil,  5, rawfree, rawnud, buildlistled, nil },
  [TOKEN_LIST]         = { false, 0,   nil,  5, listfree, rawnud, buildlistled, nil },
  [TOKEN_COMMAND]      = { false, 0,   nil,  5, commandfree, nil, nil, commandeval },

  [TOKEN_END]          = { false, 0,   nil,  0, rawfree, endnud, endled, nil },
};
