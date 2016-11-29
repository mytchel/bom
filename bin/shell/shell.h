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

#define LINEMAX 256
#define MAXARGS 64
#define MAXNESTING 10

/* Order matters, must make sure that >> comes before > and so on. */

typedef enum {
  TOKEN_PARAN_LEFT,
  TOKEN_PARAN_RIGHT,
  TOKEN_BRACE_LEFT,
  TOKEN_BRACE_RIGHT,
  TOKEN_BRACKET_LEFT,
  TOKEN_BRACKET_RIGHT,

  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_SUBST,

  TOKEN_IN,

  TOKEN_SEMI,

  TOKEN_EQUAL,
  TOKEN_ASSIGN,

  TOKEN_APPEND,
  TOKEN_OUT,

  TOKEN_OR,
  TOKEN_PIPE,

  TOKEN_AND,
  TOKEN_BG,

  TOKEN_CD,

  TOKEN_IF,
  TOKEN_ELSE,

  TOKEN_FOR,

  TOKEN_NAME,
  TOKEN_NUMBER,
  TOKEN_LIST,
  TOKEN_COMMAND,

  TOKEN_END,

  TOKEN_TYPES,
} token_t;

struct token {
  struct token *next;
  token_t type;
  void *aux;
};

struct tokentype {
  /* true for punctuators like '&', '|', '(' but false for
     'if', 'cd' etc */
  bool punc;

  size_t len;
  char *str;

  size_t bp;

  void (*free)(struct token *self);

  struct token * (*nud)(struct token *self);
  struct token * (*led)(struct token *self, struct token *left);

  int (*eval)(struct token *self, int in, int out);
};

struct listaux {
  struct token *head, *tail;
};

struct arithaux {
  struct token *left, *right;
};

typedef enum {
  COMMAND_AND,
  COMMAND_OR,
  COMMAND_BG,
  COMMAND_PIPE,
  COMMAND_SEMI,
} command_t;

struct commandaux {
  struct commandaux *parent;
  command_t type;
  char *in, *out;
  uint32_t inmode, outmode; /* Masks for standard mode */
  struct token *args;
  struct token *next;
};

struct ifaux {
  struct token *cond, *good, *fail;
};

struct token *
tokennew(token_t type);

void
tokenprint(struct token *t);

void
advance(void);

struct token *
expression(int bp);

struct token *
command(int bp);

void
interp(void);

void
setupinputfd(int nfd);

void
setupinputstring(char *s, size_t len);

extern struct tokentype types[TOKEN_TYPES];
extern struct token *token;
extern int ret;

