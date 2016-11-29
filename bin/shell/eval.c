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
#include <string.h>
#include <ctype.h>
#include <fs.h>

#include "shell.h"

static int
commandeval(struct token *self, int in, int out);

static struct commandaux *cmd = nil;

static struct commandaux *
commandnew(void)
{
  struct commandaux *c;

  c = malloc(sizeof(struct commandaux));
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

static struct commandaux *
commandpush(void)
{
  struct commandaux *c;

  c = commandnew();
  c->parent = cmd;
  cmd = c;

  return c;
}

static struct token *
buildlistled(struct token *self, struct token *left)
{
  struct listaux *l;
  struct token *n;
  
  self->next = nil;
  if (left->type == TOKEN_LIST) {
    l = (struct listaux *) left->aux;

    if (l->tail == nil) {
      l->head = l->tail = self;
    } else {
      l->tail->next = self;
      l->tail = self;
    }
    
    return left;
  } else {
    l = malloc(sizeof(struct listaux));
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
  struct listaux *l;

  l = (struct listaux *) self->aux;

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
  return nil;
}

static struct token *
endled(struct token *self, struct token *left)
{
  return nil;
}

static struct token *
bracenud(struct token *self)
{
  struct token *t, *b;
  token_t type;
  
  t = command(0);

  b = token;
  advance();

  type = b->type;
  
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
  if (self->aux != nil) {
    free(self->aux);
  }
  
  free(self);
}

static struct token *
cdnud(struct token *self)
{
  struct token *t;

  t = expression(types[self->type].bp);
  if (t == nil || t->type != TOKEN_NAME) {
    printf("usage: cd dir\n");
    return nil;
  }

  self->aux = t->aux;
  t->aux = nil;
  types[t->type].free(t);

  return self;
}

static int
cdeval(struct token *self, int in, int out)
{
  return chdir(self->aux);
}

static void
iffree(struct token *self)
{
  free(self);
}

static struct token *
ifnud(struct token *self)
{
  struct token *t;
  struct ifaux *i;
  
  printf("have if statement\n");

  t = token;
  advance();
  
  if (t->type != TOKEN_PARAN_LEFT) {
    printf("expected ( after if\n");
    return nil;
  }

  i = malloc(sizeof(struct ifaux));
  if (i == nil) {
    printf("malloc failed!\n");
    return nil;
  }

  i->cond = expression(0);

  if (token->type != TOKEN_PARAN_RIGHT) {
    printf("expected closing ), have %s\n", types[token->type].str);
    return nil;
  }

  advance();

  i->good = command(0);

  if (token->type == TOKEN_ELSE) {
    advance();
    i->fail = command(0);
  } else {
    i->fail = nil;
  }

  self->aux = i;
  
  return self;
}

static int
ifeval(struct token *self, int in, int out)
{
  return ENOIMPL;
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

static int
foreval(struct token *self, int in, int out)
{
  return ENOIMPL;
}

static void
commandfree(struct token *self)
{
  struct commandaux *c;

  c = ((struct commandaux *) self->aux);

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
  struct commandaux *c;
  token_t type;

  type = self->type;
  
  c = cmd;

  c->args = left;
  c->next = nil;

  self->type = TOKEN_COMMAND;
  self->aux = c;
  self->next = nil;

  c->next = command(types[type].bp);

  switch (type) {
  case TOKEN_BG:
    c->type = COMMAND_BG;
    break;
    
  case TOKEN_SEMI:
    c->type = COMMAND_SEMI;
    break;

  case TOKEN_AND:
    c->type = COMMAND_AND;
    goto next;
  case TOKEN_OR:
    c->type = COMMAND_OR;
    goto next;
  case TOKEN_PIPE:
    c->type = COMMAND_PIPE;

  next:
    if (c->next == nil) {
      printf("expected statement after %s.\n", types[type].str);
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

static int
convertargs(struct token *t, int argc, char *argv[])
{
  struct listaux *l;
  
  while (t != nil) {
    if (t->type == TOKEN_NAME) {
      argv[argc++] = t->aux;
    } else if (t->type == TOKEN_LIST) {
      l = (struct listaux *) t->aux;
      argc = convertargs(l->head, argc, argv);
    }

    t = t->next;
  }

  return argc;
}

static int
docmd(struct commandaux *c, int in, int out, int *pid)
{
  char *argv[MAXARGS], tmp[LINEMAX];
  int argc, r;

  *pid = 0;
  
  if (c->args == nil) {
    return ERR;

  } else if (c->args->type == TOKEN_LIST || c->args->type == TOKEN_NAME) {
    argc = convertargs(c->args, 0, argv);

    /* Fall through to fork */

  } else if (c->args->type == TOKEN_NUMBER) {
    fprintf(out, "%i\n", c->args->aux);
    return OK;
  
  } else {
    return types[c->args->type].eval(c->args, in, out);
  }

  r = fork(FORK_sngroup);
  if (r < 0) {
    fprintf(out, "fork error\n");
    return r;
  } else if (r > 0) {
    *pid = r;
    return OK;
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

  if (r == ENOFILE) {
    printf("%s : command not found\n", argv[0]);
  } else {
    printf("%s : failed to run, %i\n", argv[0], r);
  }
  
  exit(r);
 
  return 0;
}

static int
commandevalpipe(struct commandaux *c, int in, int out)
{
  int pid, r, p[2];

  if (pipe(p) != OK) {
    fprintf(out, "pipe error!\n");
    return ERR;
  }

  r = docmd(c, in, p[1], &pid);

  close(p[1]);

  if (r > 0) {
    r = types[c->next->type].eval(c->next, p[0], out);
  }

  close(p[0]);
  return r;
}

static int
commandevaland(struct commandaux *c, int in, int out)
{
  int pid, r;

  r = docmd(c, in, out, &pid);
  while (pid != 0 && wait(&r) != pid)
    ;

  if (r == OK) {
    r = types[c->next->type].eval(c->next, in, out);
  }

  return r;
}

static int
commandevalor(struct commandaux *c, int in, int out)
{
  int pid, r;

  r = docmd(c, in, out, &pid);
  while (pid != 0 && wait(&r) != pid)
    ;

  if (r != OK) {
    r = types[c->next->type].eval(c->next, in, out);
  }
  
  return r;
}

static int
commandevalbg(struct commandaux *c, int in, int out)
{
  int pid, r;

  r = docmd(c, in, out, &pid);
    
  if (c->next != nil) {
    return types[c->next->type].eval(c->next, in, out);
  } else {
    return r;
  }
}

static int
commandevalsemi(struct commandaux *c, int in, int out)
{
  int pid, r;

  r = docmd(c, in, out, &pid);
    
  while (pid != 0 && wait(&r) != pid)
    ;

  if (c->next != nil) {
    return types[c->next->type].eval(c->next, in, out);
  } else {
    return OK;
  }
}

static int
commandeval(struct token *self, int in, int out)
{
  struct commandaux *c;
  int r;

  c = self->aux;
  if (c == nil) {
    fprintf(out, "command aux is nil!\n");
    return ERR;
  }

  if (c->in != nil) {
    in = open(c->in, c->inmode|O_RDONLY);
    if (in < 0) {
      fprintf(out, "failed to open %s: %i\n", c->in, in);
      return in;
    }
  }

  if (c->out != nil) {
    r = open(c->out, c->outmode|O_WRONLY|O_CREATE, ATTR_rd|ATTR_wr);
    if (r < 0) {
      fprintf(out, "failed to open %s: %i\n", c->out, r);
      if (c->in != nil) {
	close(in);
      }
      return r;
    } else {
      out = r;
    }
  }

  switch (c->type) {
  case COMMAND_PIPE:
    r = commandevalpipe(c, in, out);
    break;
  case COMMAND_AND:
    r = commandevaland(c, in, out);
    break;
  case COMMAND_OR:
    r = commandevalor(c, in, out);
    break;
  case COMMAND_BG:
    r = commandevalbg(c, in, out);
    break;
  case COMMAND_SEMI:
    r = commandevalsemi(c, in, out);
    break;
  default:
    fprintf(out, "cmd type unknown, this should never happen!\n");
    r = ERR;
    break;
  }

  if (c->in != nil) {
    close(in);
  }

  if (c->out != nil) {
    close(out);
  }

  return r;
}

struct tokentype types[TOKEN_TYPES] = {
  [TOKEN_PARAN_LEFT]   = { true,  1,    "(", 80, rawfree, nil, nil, nil },
  [TOKEN_PARAN_RIGHT]  = { true,  1,    ")",  0, rawfree, nil, nil, nil },

  [TOKEN_BRACE_LEFT]   = { true,  1,    "{", 80, rawfree, bracenud, braceled, nil },
  [TOKEN_BRACE_RIGHT]  = { true,  1,    "}",  0, rawfree, nil, nil, nil },

  [TOKEN_BRACKET_LEFT] = { true,  1,    "[", 80, bracketleftfree, nil, nil, nil },
  [TOKEN_BRACKET_RIGHT]= { true,  1,    "]",  0, rawfree, nil, nil, nil },

  [TOKEN_EQUAL]        = { true,  2,   "==", 10, equalfree, nil, nil, nil },
  [TOKEN_ASSIGN]       = { true,  1,    "=",  9, assignfree, assignnud, assignled, nil },
  [TOKEN_PLUS]         = { true,  1,    "+", 50, rawfree, plusnud, plusled, nil },
  [TOKEN_MINUS]        = { true,  1,    "-", 50, rawfree, minusnud, minusled, nil },
  [TOKEN_SUBST]        = { true,  1,    "$",  0, substfree, substnud, substled, nil },

  [TOKEN_APPEND]       = { true,  2,   ">>", 10, rawfree, appendnud, appendled, nil },
  [TOKEN_OUT]          = { true,  1,    ">", 10, rawfree, outnud, outled, nil },
  [TOKEN_IN]           = { true,  1,    "<", 10, rawfree, innud, inled, nil },

  [TOKEN_CD]           = { false, 2,   "cd", 10, cdfree, cdnud, nil, cdeval },
  [TOKEN_IF]           = { false, 2,   "if", 10, iffree, ifnud, nil, ifeval },
  [TOKEN_ELSE]         = { false, 4, "else",  0, rawfree, nil, nil, nil },
  [TOKEN_FOR]          = { false, 3,  "for", 10, forfree, fornud, nil, foreval },

  [TOKEN_AND]          = { true,  2,   "&&",  2, rawfree, rawnud, commandled, nil },
  [TOKEN_OR]           = { true,  2,   "||",  2, rawfree, rawnud, commandled, nil },
  [TOKEN_PIPE]         = { true,  1,    "|",  2, rawfree, rawnud, commandled, nil },
  [TOKEN_BG]           = { true,  1,    "&",  1, rawfree, rawnud, commandled, nil },
  [TOKEN_SEMI]         = { true,  1,    ";",  1, rawfree, rawnud, commandled, nil },

  [TOKEN_NAME]         = { false, 0, "name",  5, namefree, rawnud, buildlistled, nil },
  [TOKEN_NUMBER]       = { false, 0,  "num",  5, rawfree, rawnud, buildlistled, nil },
  [TOKEN_LIST]         = { false, 0, "list",  5, listfree, rawnud, buildlistled, nil },
  [TOKEN_COMMAND]      = { false, 0,  "cmd",  5, commandfree, nil, nil, commandeval },

  [TOKEN_END]          = { false, 0,"(end)",  0, rawfree, endnud, endled, nil },
};

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
  struct commandaux *p;

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
    cmd->type = COMMAND_SEMI;
    cmd->args = t;

    commandpop();

    return w;
  }
}

