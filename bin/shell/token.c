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

void
tokenfree(struct token *t)
{
  struct token *n;

  n = t->next;
  if (t->type == TOKEN_NAME) {
    free(t->val.str);
  }

  free(t);

  if (n != nil) {
    tokenfree(n);
  }
}

static struct token *
punctuatornew(token_t type)
{
  struct token *t;

  t = malloc(sizeof(struct token));
  if (t == nil) {
    printf("malloc failed!\n");
    exit(ENOMEM);
  }

  t->type = type;

  return t;
}

static token_t
punctuator(char *s, size_t max, size_t *l)
{
  *l = 1;
  
  switch (s[0]) {
  case '(':
    return TOKEN_PARAN_LEFT;
  case ')':
    return TOKEN_PARAN_RIGHT;
  case '{':
    return TOKEN_BRACE_LEFT;
  case '}':
    return TOKEN_BRACE_RIGHT;
  case '[':
    return TOKEN_BRACKET_LEFT;
  case ']':
    return TOKEN_BRACKET_RIGHT;
  case '+':
    return TOKEN_PLUS;
  case '-':
    return TOKEN_MINUS;
  case '^':
    return TOKEN_CARET;
  case '$':
    return TOKEN_SUBST;
  case '*':
    return TOKEN_STAR;
  case '@':
    return TOKEN_AT;

  case '=':
    if (max > 1 && s[1] == '=') {
      *l = 2;
      return TOKEN_EQUAL;
    } else {
      return TOKEN_ASSIGN;
    }

  case '&':
    if (max > 1 && s[1] == '&') {
      *l = 2;
      return TOKEN_AND;
    } else {
      return TOKEN_BG;
    }

  case '|':
    if (max > 1 && s[1] == '|') {
      *l = 2;
      return TOKEN_OR;
    } else {
      return TOKEN_PIPE;
    }

  case '>':
    if (max > 1 && s[1] == '>') {
      *l = 2;
      return TOKEN_APPEND;
    } else {
      return TOKEN_OUT;
    }

  case '<':
    return TOKEN_IN;

  case '\n':
  case ';':
    return TOKEN_END;
  default:
    return 0;
  }
}

struct token *
tokenize(char *s, size_t max)
{
  struct token tokens, *t;
  size_t i, j, l, qs;
  token_t type;
  int num;
  bool q;

  t = &tokens;

  i = 0;
  while (i < max) {
    if (s[i] == '\\') {
      i += 2;
      continue;
    } else if (s[i] != '\n' && isspace(s[i])) {
      i++;
      continue;
    }

    type = punctuator(&s[i], max - i, &l);
    if (type != TOKEN_NONE) {
      t->next = punctuatornew(type);
      t = t->next;
      i += l;
      continue;
    }

    t->next = malloc(sizeof(struct token));
    if (t->next == nil) {
      printf("malloc failed!\n");
      exit(ENOMEM);
    }

    t = t->next;

    t->type = TOKEN_NAME;

    t->val.str = malloc(sizeof(char) * LINEMAX);
    if (t->val.str == nil) {
      printf("malloc failed!\n");
      exit(ENOMEM);
    }

    num = 1;
    q = false;
    j = qs = 0;
    while (i + qs + j < max) {
      if (s[i + qs + j] == '\'') {
	num = 0;
	
	qs++;
	if (i + qs + j < max && s[i + qs + j] == '\'') {
	  t->val.str[j++] = '\'';
	} else {
	  q = !q;
	}
      } else if (!q && isspace(s[i + qs + j])) {
	break;
      } else if (!q && (type = punctuator(&s[i + qs + j],
					  max - i - qs - j, &l))
		 != TOKEN_NONE) {
	break;
      } else {
	t->val.str[j] = s[i + qs + j];
	if (num == 1 && (t->val.str[j] > '9' || t->val.str[j] < '0')) {
	  num = 0;
	}
	j++;
      }
    }

    t->val.str[j] = 0;
    if (num == 1) {
      num = strtol(t->val.str, nil, 10);
      free(t->val.str);
      t->val.num = num;
      t->type = TOKEN_NUMBER;
    }

    i += j + qs;
  }

  t->next = malloc(sizeof(struct token));
  if (t->next == nil) {
    printf("malloc failed!\n");
    exit(ENOMEM);
  }

  t->next->type = TOKEN_END;
  t->next->next = nil;
  
  return tokens.next;
}
