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
  TYPE_free = 0,
  TYPE_atom,
  /* Same as atom but need different type for >, >>, < etc. */
  TYPE_sym,
  TYPE_list,
  TYPE_join,
  TYPE_delim,
} type_t;

typedef enum {
  DELIM_end,
  DELIM_pipe,
  DELIM_bg,
  DELIM_and,
  DELIM_or,
} delim_t;

struct atom {
  type_t type;
  struct atom *next;

  union {
    struct { /* atom */
      char *str;
      size_t len;
    } a;

    struct { /* list */
      struct atom *head;
    } l;

    struct { /* join */
      struct atom *a, *b;
    } j;

    struct { /* delim */
      delim_t type;
    } d;
  };
};


struct atom *
flatten(struct atom *atom);

struct atom *
atomnew(type_t type);

void
atomfree(struct atom *a);

void
atomfreel(struct atom *a);

struct atom *
parseatoml(char *buf, size_t max);

int
atomeval(struct atom *atoms);

bool
runfunc(struct atom *cmd, struct atom *rest, struct atom *delim);

void
interactive(void);

extern int ret;

