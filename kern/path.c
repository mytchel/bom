/*
 *
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

#include "head.h"

static struct path *
strtopathh(struct path *prev, const char *str)
{
  struct path *p;
  int i, j;
	
  for (i = 0; str[i] && str[i] != '/'; i++);
  if (i == 0 && str[i]) {
    return strtopathh(prev, str + 1);
  } else if (i == 0) {
    return nil;
  } else if (i >= FS_NAME_MAX) {
    return nil;
  }
	
  p = malloc(sizeof(struct path));
  /* Best to cause some more major issue in the case of a failed
   * malloc, as I have no way to check if there was an error
   * (or end of the path).
   */
	
  for (j = 0; j < i; j++)
    p->s[j] = str[j];
  p->s[j] = 0;

  p->prev = prev;
  p->next = strtopathh(p, str + i);

  return p;
}

struct path *
strtopath(const char *str)
{
  return strtopathh(nil, str);
}

char *
pathtostr(struct path *p, size_t *n)
{
  char *str;
  size_t len, i;
  struct path *pp;
	
  len = 0;
  if (p == nil) {
    len = 1;
  }

  for (pp = p; pp != nil; pp = pp->next)
    len += strlen(pp->s) + 1;

  str = malloc(len);
  if (str == nil) {
    return nil;
  }
		
  if (n != nil)
    *n = len;
		
  i = 0;
  for (pp = p; pp != nil; pp = pp->next) {
    len = strlen(pp->s);
    memmove(&str[i], pp->s, len);
    i += len;
    if (pp->next)
      str[i++] = '/';
  }
	
  str[i] = 0;
		
  return str;
}

struct path *
realpath(struct path *po, const char *str)
{
  struct path *pstr, *pp, *pt, *path;
  
  pstr = strtopath(str);
  if (str[0] == '/' || po == nil) {
    path = pstr;
  } else {
    path = pathcopy(po);

    for (pp = path; pp != nil && pp->next != nil; pp = pp->next);
    pp->next = pstr;
    if (pstr != nil) {
      pstr->prev = pp;
    }
  }

  pp = path;
  while (pp != nil) {
    if (strcmp(pp->s, "..")) {
      /* Remove current and previous part */

      memmove(pp->s, ".", 2);
      pt = pp->prev;
      if (pt != nil) {
	memmove(pt->s, ".", 2);
	pp = pt;
      }	
    } else if (strcmp(pp->s, ".")) {
      /* Remove current part */
      pt = pp;
			
      if (pp->prev != nil) {
	pp->prev->next = pp->next;
      } else {
	path = pp->next;
      }
			
      if (pp->next != nil) {
	pp->next->prev = pp->prev;
      }
			
      pp = pp->next;
			
      free(pt);
    } else {
      pp = pp->next;
    }
  }

  return path;
}

struct path *
pathcopy(struct path *o)
{
  struct path *n, *nn;
	
  if (o == nil)
    return nil;
	
  n = malloc(sizeof(struct path));
  nn = n;
  nn->prev = nil;
	
  while (o != nil) {
    memmove(nn->s, o->s, FS_NAME_MAX);

    o = o->next;
    if (o != nil) {
      nn->next = malloc(sizeof(struct path));
      nn->next->prev = nn;
      nn = nn->next;
    } 
  }
	
  nn->next = nil;
	
  return n;
}

void
pathfree(struct path *p)
{
  if (p == nil)
    return;
		
  pathfree(p->next);
	
  free(p);
}
