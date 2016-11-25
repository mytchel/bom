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

static struct token *
readtokens(void);

struct token *token;

static size_t max, offset = 0;
static char base[LINEMAX];
static char *s;
static int fd;

void
setupinputstring(char *str, size_t len)
{
  fd = ERR;

  max = len;
  offset = 0;
  s = base;

  memmove(base, str, len);

  token = readtokens();
}

void
setupinputfd(int nfd)
{
  fd = nfd;
  max = offset = 0;
  s = base;

  token = readtokens();
}

static size_t
readlen(size_t n)
{
  int r;

  if (n <= max) {
    return max;
  } else if (n >= sizeof(base)) {
    printf("can not read this much in one go\n");
    return ERR;
  } else if (fd == ERR) {
    return max;
  }
 
  if (offset + n > LINEMAX) {
    memmove(base, base + offset, max);
    offset = 0;
    s = base;
  }
 
  r = read(fd, s + max, n - max);
  if (r < 0) {
    printf("error reading input : %i\n", r);
  } else {
    max += r;
  }

  return max;
}

static void
eatlen(size_t n)
{
  if (n > max) {
    printf("need to read more before you can eat\n");
    return;
  }
  
  offset += n;
  
  if (offset >= LINEMAX) {
    printf("need to do some moving\n");
    memmove(base, &base[offset], max);
    offset = 0;
  }

  max -= n;
  s = &base[offset];
  s[max] = 0;
}

static struct token *
literal(struct token *t, char *s, size_t l, bool num)
{
  s[l-1] = 0;

  if (num) {
    t->type = TOKEN_NUMBER;
    t->aux = (void *) strtol(s, nil, 10);
  } else {
    t->type = TOKEN_NAME;
    t->aux = malloc(sizeof(char) * l);
    if (t->aux == nil) {
      printf("malloc failed!\n");
      exit(ENOMEM);
    } else {
      strlcpy(t->aux, s, l);
    }
  }

  return t;
}

static struct token *
readtokens(void)
{
  char str[LINEMAX];
  struct token *t;
  token_t type;
  bool q, num;
  size_t j;

  t = malloc(sizeof(struct token));
  if (t == nil) {
    printf("malloc failed.\n");
    exit(ENOMEM);
  }

  t->next = nil;

  q = false;
  num = true;
  j = 0;

  while (readlen(1) > 0) {
    if (s[0] == '\\') {
      if (readlen(2) < 2 || s[1] != '\n') {
	printf("expected new line after \\\n");
	break;
      } else {
	eatlen(2);
	continue;
      }

    } else if (s[0] == '\'') {
      num = false;
      if (readlen(2) < 2 || s[1] != '\'') {
	q = !q;
	eatlen(1);
      } else {
	str[j++] = '\'';
	eatlen(2);
      } 

      continue;
    } else if (!q && s[0] == '\n') {
      eatlen(1);

      if (j > 0) {
	t->next = tokennew(TOKEN_SEMI);
	return literal(t, str, j + 1, num);
      } else {
	t->type = TOKEN_SEMI;
	t->aux = nil;
	return t;
      }
      
    } else if (!q && isspace(s[0])) {
      eatlen(1);
      if (j > 0) {
	return literal(t, str, j + 1, num);
      } else {
	continue;
      }
    }

    for (type = 0; !q && type < TOKEN_TYPES; type++) {
      if (!types[type].punc || types[type].str == nil) continue;
      if (readlen(types[type].len) < types[type].len) {
	continue;
      }

      if (strncmp(s, types[type].str, types[type].len)) {
	eatlen(types[type].len);

	if (j > 0) {
	  t->next = tokennew(type);
	  return literal(t, str, j + 1, num);
	} else {
	  t->type = type;
	  t->aux = nil;
	  return t;
	}
      }
    }

    if (j >= LINEMAX) {
      printf("token length too long!\n");
      exit(ENOMEM);
    }
    
    str[j++] = s[0];

    if (num && (s[0] < '0' || s[0] > '9')) {
      num = false;
    }
    
    eatlen(1);
  }

  if (j > 0) {
    t->next = tokennew(TOKEN_SEMI);
    return literal(t, str, j + 1, num);
  } else {
    t->type = TOKEN_END;
    t->aux = nil;
    return t;
  }
}

struct token *
tokennew(token_t type)
{
  struct token *t;

  t = malloc(sizeof(struct token));
  if (t == nil) {
    printf("malloc failed.\n");
    exit(ENOMEM);
  }
	  
  t->next = nil;
  t->type = type;
  t->aux = nil;
  return t;
}

void
tokenprint(struct token *t)
{
  struct command *c;
  struct list *l;

  if (t->type == TOKEN_LIST) {
    printf("(");
    l = (struct list *) t->aux;
    t = l->head;
    while (t != nil) {
      tokenprint(t);
      t = t->next;
    }
    printf(")");
  } else if (t->type == TOKEN_COMMAND) {
    c = t->aux;
    printf(" {command  %s [> %s], [< %s]: ", types[c->type].str, c->out, c->in);
    tokenprint(c->args);
    printf(" : ");
    if (c->next == nil) {
      printf(" nil ");
    } else {
      tokenprint(c->next);
    }
    printf(" } ");
  } else if (t->type == TOKEN_NUMBER) {
    printf(" %i ", t->aux);
  } else if (t->type == TOKEN_NAME) {
    printf(" '%s' ", t->aux);
  } else {
    printf(" %s ", types[t->type].str);
  }
}

void
advance(void)
{
  token = token->next;
  if (token == nil) {
    token = readtokens();
  }
}

