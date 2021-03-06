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
#include <string.h>

#include "shell.h"

int ret = 0;

void
shiftstringright(char *s, size_t max, size_t i)
{
  size_t j;
  
  j = max - 1;
  while (j > i) {
    s[j] = s[j-1];
    j--;
  }
}

void
shiftstringleft(char *s, size_t max)
{
  size_t j;

  j = 0;
  while (j < max) {
    s[j] = s[j + 1];
    j++;
  }
}

void
interp(void)
{
  char line[LINEMAX];
  struct token *t;
  size_t b, m, i;
  int p[2];
  bool q;
  char c;
  int r;
  
  while (true) {
  prompt:
    write(STDOUT, "; ", 2);

    q = false;
    b = m = i = 0;
    while ((r = read(STDIN, &c, sizeof(char))) > 0) {
      if (c == 127 || c == 8) {
	if (i > 0) {
	  i--;
	  shiftstringleft(&line[i], m - i);
	}
	continue;

      } else if (i == sizeof(line)) {
	printf("line length exceded!\n");
	goto prompt;
      }

      line[i] = c;
      i++;

      if (i > m) {
	m = i;
      }
      
      if (c == '(' || c == '{' || c == '[') {
	b++;
      } else if (c == ')' || c == '}' || c == ']') {
	b--;
      } else if (c == '\'') {
	q = !q;
      } else if (!q && b == 0 && c == '\n'
		 && (i == 0 || line[i-1] != '\\')) {
	break;
      }
    }
    
    if (r == 0) {
      exit(OK);
    } else if (r < 0) {
      printf("error reading input: %i\n", r);
      exit(r);
    }

    if (pipe(p) != OK) {
      printf("pipe error\n");
      exit(ERR);
    }

    setupinputstring(line, m);

    while ((t = command(0)) != nil) {
      ret = types[t->type].eval(t, STDIN, STDOUT);
      types[t->type].free(t);
    }
  }
}
