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

static struct {
  struct path *free;
  struct lock lock;
} pathalloc = {0};

#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

static struct path *
strtopathh(struct path *p, const char *str)
{
  struct path *n;
  int i, j;

  for (i = 0; str[i] && str[i] != '/'; i++);
  if (i == 0 && str[i]) {
    return strtopathh(p, str + 1);
  } else if (i == 0) {
    return nil;
  } else if (i >= NAMEMAX) {
    return nil;
  }

  if (str[0] == '.') {
    if (str[1] == '.' && (str[2] == '/' || str[2] == 0)) {
      /* Remove current and prev */

      if (p == nil) {
	return strtopathh(n, str + 2);
      } else {
	n = p->prev;
      }

      lock(&pathalloc.lock);
      p->next = pathalloc.free;
      pathalloc.free = p;
      unlock(&pathalloc.lock);

      if (n != nil) {
	n->next = strtopathh(n, str + 2);
      } else {
	return strtopathh(nil, str + 2);
      }
    } else if (str[1] == '/' || str[1] == 0) {
      /* Remove current */
      return strtopathh(p, str + 1);
    }
  }
  
  lock(&pathalloc.lock);

  n = pathalloc.free;
  if (n == nil) {
    n = malloc(sizeof(struct path));
    /* Best to cause some more major issue in the case of a failed
     * malloc, as I have no way to check if there was an error
     * (or end of the path).
     */
  } else {
    pathalloc.free = n->next;
  }

  unlock(&pathalloc.lock);
	
  for (j = 0; j < i; j++)
    n->s[j] = str[j];

  n->s[j] = 0;

  n->prev = p;
  n->next = strtopathh(n, str + i);

  return n;
}

struct path *
strtopath(struct path *p, const char *str)
{
  struct path *path;

  if (str[0] == '/' || p == nil) {
    return strtopathh(nil, str);
  } else {
    path = pathcopy(p);
    
    for (p = path; p != nil && p->next != nil; p = p->next)
      ;

    p->next = strtopathh(p, str);

    return path;
  }
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
pathcopy(struct path *o)
{
  struct path *n, *nn;
	
  if (o == nil)
    return nil;
	
  lock(&pathalloc.lock);
  n = pathalloc.free;
  if (n == nil) {
    n = malloc(sizeof(struct path));
  } else {
    pathalloc.free = n->next;
  }
      
  unlock(&pathalloc.lock);

  nn = n;
  nn->prev = nil;
	
  while (o != nil) {
    memmove(nn->s, o->s, NAMEMAX);

    o = o->next;
    if (o != nil) {
      lock(&pathalloc.lock);

      nn->next = pathalloc.free;
      if (nn->next == nil) {
	nn->next = malloc(sizeof(struct path));
      } else {
	pathalloc.free = nn->next->next;
      }
      
      unlock(&pathalloc.lock);

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

  lock(&pathalloc.lock);

  p->next = pathalloc.free;
  pathalloc.free = p;

  unlock(&pathalloc.lock);
}
