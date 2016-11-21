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

typedef enum {
  TOKEN_NONE,

  TOKEN_NAME,
  TOKEN_NUMBER,
  TOKEN_LIST,

  TOKEN_PARAN_LEFT,
  TOKEN_PARAN_RIGHT,
  TOKEN_BRACE_LEFT,
  TOKEN_BRACE_RIGHT,
  TOKEN_BRACKET_LEFT,
  TOKEN_BRACKET_RIGHT,
  TOKEN_SUBST,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_CARET,
  TOKEN_STAR,
  TOKEN_AT,
  TOKEN_ASSIGN,
  TOKEN_EQUAL,
  TOKEN_AND,
  TOKEN_BG,
  TOKEN_OR,
  TOKEN_PIPE,
  TOKEN_APPEND,
  TOKEN_OUT,
  TOKEN_IN,
  TOKEN_END,
} token_t;

struct token {
  struct token *next;

  token_t type;

  union {
    char *str;
    int num;
    struct token *head;
  } val;
};

struct token *
tokenize(char *, size_t);

void
tokenfree(struct token *);

void
advance(void);

struct token *
expression(int bp);

void
eval(void);

void
interp(void);

extern struct token *token;
extern int ret;
