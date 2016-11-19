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

static struct atom atomsfree[LINEMAX] = {{0}};

#if 0
static void
atomprint(struct atom *a)
{
  struct atom *b;
  
  switch (a->type) {
  case TYPE_free:
    return;

  case TYPE_sym:
    printf("$%s$", a->a.str);
    break;
    
  case TYPE_atom:
    printf("%s", a->a.str);
    break;

  case TYPE_join:
    printf("[");
    atomprint(a->j.a);
    printf("^");
    atomprint(a->j.b);
    printf("]");
    break;

  case TYPE_list:
    printf("(");

    for (b = a->l.head; b != nil; b = b->next) {
      printf(" ");
      atomprint(b);
      printf(" ");
    }

    printf(")");
    break;

  case TYPE_delim:
    printf(";");
    break;
  }
}
#endif

static bool
isdelim(char c)
{
  switch (c) {
  case ';':
  case '&':
  case '|':
    return true;
  default:
    return false;
  }
}

static struct atom *
substitute(char *name)
{
  return nil;
}

static struct atom *
patternmatch(struct atom *old)
{
  struct atom *a;

  a = atomnew(TYPE_atom);
  if (a == nil) {
    printf("memory error.\n");
    return nil;
  }

  a->a.len = old->a.len;
  a->a.str = malloc(a->a.len + 1);
  if (a->a.str == nil) {
    printf("memory error.\n");
    atomfree(a);
    return nil;
  }

  memmove(a->a.str, old->a.str, a->a.len);
  a->a.str[a->a.len] = 0;

  return a;
}

struct atom *
atomnew(type_t type)
{
  int i;

  for (i = 0; i < sizeof(atomsfree) / sizeof(struct atom); i++) {
    if (atomsfree[i].type == TYPE_free) {
      atomsfree[i].type = type;
      return &atomsfree[i];
    }
  }

  return nil;
}

void
atomfree(struct atom *a)
{
  if (a == nil) {
    return;
  }

  switch (a->type) {
  case TYPE_sym:
  case TYPE_atom:
    free(a->a.str);
    break;
    
  case TYPE_join:
    atomfree(a->j.a);
    atomfree(a->j.b);
    break;
    
  case TYPE_list:
    atomfreel(a->l.head);
    break;

  default:
    break;
  }
  
  memset(a, 0, sizeof(struct atom));
}

void
atomfreel(struct atom *a)
{
  struct atom *n;

  while (a != nil) {
    n = a->next;
    atomfree(a);
    a = n;
  }
}

static struct atom *
joinlists(struct atom *a, struct atom *b)
{
  struct atom *j;
  
  j = atomnew(TYPE_join);
  if (j == nil) {
    printf("memory error.\n");
    atomfree(a);
    atomfree(b);
    return nil;
  }

  j->j.a = a;
  j->j.b = b;

  return j;
}

static size_t
matchingpos(char *buf, size_t max, char o, char c)
{
  size_t i, b;
  
  b = 1;
  for (i = 1; b > 0 && i < max; i++) {
    if (buf[i] == o) {
      b++;
    } else if (buf[i] == c) {
      b--;
    }
  }

  if (b != 0) {
    return 0;
  } else {
    return i - 1;
  }
}

static struct atom *
quoted(char *buf, size_t max, size_t *i)
{
  struct atom *a;
  size_t j;
  
  a = atomnew(TYPE_atom);
  if (a == nil) {
    printf("memory error.\n");
    return nil;
  }

  for (*i = 1; *i < max; (*i)++) {
    if (buf[*i] == '\'') {
      if (*i < max && buf[*i+1] == '\'') {
	continue;
      } else {
	break;
      }
    }
  }

  a->a.str = malloc(*i + 1);
  if (a->a.str == nil) {
    printf("memory error.\n");
    atomfree(a);
    return nil;
  }

  j = 0;
  for (*i = 1; *i < max; (*i)++) {
    if (buf[*i] == '\'') {
      if (*i < max && buf[*i+1] == '\'') {
	a->a.str[j++] = '\'';
      } else {
	break;
      }
    } else {
      a->a.str[j++] = buf[*i];
    }
  }

  (*i)++;

  a->a.str[j] = 0;
  a->a.len = j;

  return a;
}

static struct atom *
normal(char *buf, size_t max, size_t *i)
{
  struct atom *a;
  
  a = atomnew(TYPE_atom);
  if (a == nil) {
    printf("memory error.\n");
    return nil;
  }

  for (*i = 1; *i < max; (*i)++) {
    if (buf[*i] == '^' ||
	buf[*i] == '\'' ||
	buf[*i] == '$') {
      break;
    }
  }

  a->a.len = *i;
  a->a.str = malloc(a->a.len + 1);
  if (a->a.str == nil) {
    printf("memory error.\n");
    atomfree(a);
    return nil;
  }

  memmove(a->a.str, buf, a->a.len);
  a->a.str[a->a.len] = 0;

  return a;
}

static struct atom *
parttolist(char *buf, size_t max)
{
  struct atom *a, *t;
  size_t i;

  switch (buf[0]) {
  case '(':
    i = matchingpos(buf, max, '(', ')');
    if (i == 0) {
      printf("syntax error, mismatching brackets : %s\n", buf);
      return nil;
    }

    a = parseatoml(buf + 1, i - 1);
    i = i + 1;
    break;

  case '{':
    i = matchingpos(buf, max, '{', '}');
    if (i == 0) {
      printf("syntax error, mismatching brackets : %s\n", buf);
      return nil;
    }

    a = parseatoml(buf + 1, i - 1);
    i = i + 1;
    break;

  case '[':
    i = matchingpos(buf, max, '[', ']');
    if (i == 0) {
      printf("syntax error, mismatching brackets : %s\n", buf);
      return nil;
    }

    a = atomnew(TYPE_atom);
    if (a == nil) {
      printf("memory error.\n");
      return nil;
    }

    a->a.len = i;
    a->a.str = malloc(a->a.len + 1);
    if (a->a.str == nil) {
      printf("memory error.\n");
      atomfree(a);
      return nil;
    }

    memmove(a->a.str, buf, i);
    a->a.str[i] = 0;
    break;

  case '\'':
    a = quoted(buf, max, &i);
    break;

  default:
    a = normal(buf, max, &i);
    break;
  }

  if (a == nil) {
    return nil;
  } else if (i == max) {
    return a;
  } else if (buf[0] == '\'' || buf[0] == '[' || buf[i] == '$' || buf[i] == '\'') {
    t = parttolist(buf + i, max - i);
    if (t == nil) {
      atomfree(a);
      return nil;
    }

    return joinlists(a, t);

  } else if (buf[i] == '^') {
    t = parttolist(buf + i + 1, max - i - 1);
    if (t == nil) {
      atomfree(a);
      return nil;
    }

    return joinlists(a, t);
  }

  printf("syntax error, expected ^ : %s\n", buf);
  atomfree(a);
  return nil;
}

static size_t
endofpart(char *buf, size_t max)
{
  char bracks[10];
  size_t j, b;
  bool q;

  j = 0; 
  b = 0;
  q = false;
  while (j < max) {
    if (!q &&
	(buf[j] == '(' ||
	 buf[j] == '{' ||
	 buf[j] == '[')) {

      bracks[b++] = buf[j];
    } else if (!q && buf[j] == ')') {
      if (b > 0 && bracks[b-1] == '(') {
	b--;
      } else {
	printf("syntax error, unexpected ) in : %s\n", buf);
	return 0;
      }
    } else if (!q && buf[j] == '}') {
      if (b > 0 && bracks[b-1] == '{') {
	b--;
      } else {
	printf("syntax error, unexpected } in : %s\n", buf);
	return 0;
      }
    } else if (!q && buf[j] == ']') {
      if (b > 0 && bracks[b-1] == '[') {
	b--;
      } else {
	printf("syntax error, unexpected ] in : %s\n", buf);
	return 0;
      }
    } else if (buf[j] == '\'') {
      q = !q;
    } else if (!q && b == 0 &&
	       (isspace(buf[j]) ||
		isdelim(buf[j]) ||
		buf[j] == '>' ||
		buf[j] == '<')) {
      break;
    } 

    j++;
  }

  if (b > 0 || q) {
    printf("syntax error in : %s\n", buf);
    return 0;
  }

  return j;
}

struct atom *
parseatoml(char *buf, size_t max)
{
  struct atom *head, *p, *t;
  size_t i, j;
  
  head = atomnew(TYPE_list);
  if (head == nil) {
    printf("error allocating atom\n");
    return nil;
  }

  i = j = 0;
  p = nil;
  while (i < max) {
    if (isspace(buf[i])) {
      i++;
      continue;
    }

    if (isdelim(buf[i])) {
      t = atomnew(TYPE_delim);
      if (t == nil) {
	printf("memory error.\n");
	atomfree(head);
	return nil;
      }

      if (buf[i] == '&' && buf[i+1] == '&') {
	t->d.type = DELIM_and;
	j = 2;
      } else  if (buf[i] == '&') {
	t->d.type = DELIM_bg;
	j = 1;
      } else if (buf[i] == '|' && buf[i+1] == '|') {
	t->d.type = DELIM_or;
	j = 2;
      } else if (buf[i] == '|') {
	t->d.type = DELIM_pipe;
	j = 1;
      } else  if (buf[i] == ';') {
	t->d.type = DELIM_end;
	j = 1;
      }

    } else if (buf[i] == '>' || buf[i] == '<') {
      j = 1;
      while (buf[i+j] == buf[i])
	j++;

      t = parttolist(buf + i, j);
      if (t == nil) {
	atomfree(head);
	return nil;
      }
     
      t->type = TYPE_sym;

    } else {
      j = endofpart(buf + i, max - i);
      if (j == 0) {
	atomfree(head);
	return nil;
      }
    
      t = parttolist(buf + i, j);
      if (t == nil) {
	atomfree(head);
	return nil;
      }
    }

    if (p == nil) {
      head->l.head = t;
    } else {
      p->next = t;
    }

    p = t;
    i += j;
  }

  return head;
}

static struct atom *
flattenlist(struct atom *atom)
{
  struct atom *head, *a, *ap, *an;

  head = ap = nil;
  for (a = atom->l.head; a != nil; a = a->next) {
    an = flatten(a);
    if (an == nil) {
      atomfreel(head);
      return nil;
    }

    if (ap == nil) {
      head = an;
    } else {
      ap->next = an;
    }

    ap = an;
    while (ap->next != nil) {
      ap = ap->next;
    }
  }

  return head;
}

static struct atom *
flattenjoin(struct atom *atom)
{
  struct atom *a, *b, *aa, *bb;
  struct atom *head, *n, *p;

  a = flatten(atom->j.a);
  b = flatten(atom->j.b);

  if (a == nil || b == nil) {
    atomfree(a);
    atomfree(b);
    return nil;
  }

  head = p = nil;

  for (aa = a; aa != nil; aa = aa->next) {
    for (bb = b; bb != nil; bb = bb->next) {
      n = atomnew(TYPE_atom);
      if (n == nil) {
	printf("memory error.\n");
	atomfree(a);
	atomfree(b);
	atomfreel(head);
	return nil;
      }

      n->a.len = aa->a.len + bb->a.len;
      n->a.str = malloc(n->a.len + 1);
      if (n->a.str == nil) {
	printf("memory error.\n");
	atomfree(a);
	atomfree(b);
	atomfree(n);
	atomfreel(head);
	return nil;
      }

      memmove(n->a.str, aa->a.str, aa->a.len);
      memmove(n->a.str + aa->a.len, bb->a.str, bb->a.len);
      n->a.str[n->a.len] = 0;

      if (p == nil) {
	head = n;
      } else {
	p->next = n;
      }

      p = n;
    }
  }

  return head;
}

struct atom *
flatten(struct atom *atom)
{
  struct atom *a = nil;

  switch (atom->type) {
  case TYPE_list:
    return flattenlist(atom);

  case TYPE_join:
    return flattenjoin(atom);

  case TYPE_atom:
    if (atom->a.str[0] == '$') {
      a = substitute(atom->a.str);
      if (a == nil) {
	printf("variable %s not found!\n", atom->a.str);
      }
    } else {
      a = patternmatch(atom);
    }

    return a;

  case TYPE_sym:
    a = atomnew(TYPE_sym);
    if (a == nil) {
      printf("memory error.\n");
      return nil;
    }
      
    a->a.len = atom->a.len;
    a->a.str = malloc(a->a.len + 1);
    if (a->a.str == nil) {
      printf("memory error.\n");
      atomfree(a);
      return nil;
    }

    memmove(a->a.str, atom->a.str, a->a.len);
    a->a.str[a->a.len] = 0;
    
    return a;

  case TYPE_delim:
    a = atomnew(TYPE_delim);
    if (a == nil) {
      printf("memory error.\n");
      return nil;
    }

    a->d.type = atom->d.type;
    return a;

  default:
    return nil;
  }
}

void
runcmd(struct atom *cmd, struct atom *rest, struct atom *delim,
       int in, int out)
{
  struct atom *a, *ap, *aa;
  char *argv[MAXARGS], tmp[LINEMAX];
  int r;

  ap = cmd;
  for (a = rest; a != delim; a = a->next) {
    aa = flatten(a);
    if (aa == nil) {
      exit(ERR);
    }

    while (ap->next != nil) {
      ap = ap->next;
    }

    ap->next = aa;
  }
 
  r = 0;

  for (a = cmd; a != nil; a = a->next) {
    if (a->type != TYPE_sym) {
      argv[r++] = a->a.str;
    } else if (strcmp(a->a.str, ">>")) {
      if (a->next == nil) {
	printf("expected file name after >>\n");
	exit(ERR);
      } else {
	a = a->next;
      }

      out = open(a->a.str, O_WRONLY|O_APPEND|O_CREATE, ATTR_rd|ATTR_wr);
      if (out < 0) {
	printf("error opening %s : %i\n", a->a.str, out);
	exit(out);
      }

    } else if (strcmp(a->a.str, ">")) {
      if (a->next == nil) {
	printf("expected file name after >\n");
	exit(ERR);
      } else {
	a = a->next;
      }

      out = open(a->a.str, O_WRONLY|O_TRUNC|O_CREATE, ATTR_rd|ATTR_wr);
      if (out < 0) {
	printf("error opening %s : %i\n", a->a.str, out);
	exit(out);
      } 
    } else if (strcmp(a->a.str, "<")) {
      if (a->next == nil) {
	printf("expected file name after <\n");
	exit(ERR);
      } else {
	a = a->next;
      }

      in = open(a->a.str, O_RDONLY);
      if (in < 0) {
	printf("error opening %s : %i\n", a->a.str, in);
	exit(in);
      } 
    } else {
      printf("unknown symbol %s\n", a->a.str);
      exit(ERR);
    }
  }

  if (in != STDIN) {
    dup2(in, STDIN);
    close(in);
  }

  if (out != STDOUT) {
    dup2(out, STDOUT);
    close(out);
  }

  if (argv[0][0] == '.' || argv[0][0] == '/') {
    r = exec(argv[0], r, argv);
  } else {
    strlcpy(tmp, "/bin/", LINEMAX);
    strlcpy(tmp + 5, argv[0], LINEMAX - 5);
    r = exec(tmp, r, argv);
  }

  printf("%s: exec failed : %i\n", argv[0], r);
  exit(r);
}

int
atomeval(struct atom *atoms)
{
  struct atom *head, *next;
  int pid, p[2], in, out;
 
  in = STDIN;
  out = STDOUT;

  while (atoms != nil) {
    /* Find the end of the statment, next will either be a 
     * deliminator or nil.
     */
    next = atoms;
    while (next != nil && next->type != TYPE_delim) {
      next = next->next;
    }

    head = flatten(atoms);
    if (head == nil) {
      return ERR;
    }

    if (runfunc(head, atoms->next, next)) {
      /* the func should free head and any it creates */
      if (next == nil) {
	break;
      } else {
	atoms = next->next;
	continue;
      }
    }

    if (next != nil && next->d.type == DELIM_pipe) {
      if (pipe(p) != OK) {
	printf("pipe error.\n");
	return ERR;
      } else {
	out = p[1];
      }
    }

    pid = fork(FORK_sngroup);
    if (pid == 0) {
      runcmd(head, atoms->next, next, in, out);
      /* Never reached */
    }

    atomfreel(head);

    if (next == nil ||
	next->d.type == DELIM_end ||
	next->d.type == DELIM_or ||
	next->d.type == DELIM_and) {

      while (wait(&ret) != pid)
	;
    }

    if (in != STDIN) {
      close(in);
      in = STDIN;
    }

    if (out != STDOUT) {
      close(out);
      out = STDOUT;
    }
    
    if (next == nil) {
      break;
    }

    switch (next->d.type) {
    case DELIM_pipe:
      in = p[0];
      break;

    case DELIM_or:
      if (ret == OK) {
	return OK;
      }
      break;
 
    case DELIM_and:
      if (ret != OK) {
	return ret;
      }

      break;

    default:
      break;
    }

    atoms = next->next;
  }
  
  return 0;
}
