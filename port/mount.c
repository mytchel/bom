/*
 *   Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "head.h"

static bool
respread(struct chan *c, struct response *resp)
{
  size_t size;

  size = sizeof(resp->rid)
    + sizeof(resp->ret)
    + sizeof(resp->lbuf);
  
  if (piperead(c, (void *) resp, size) != size) return false;

  if (resp->lbuf > 0) {
    resp->buf = malloc(resp->lbuf);
    if (resp->buf == nil) {
      printf("kproc mount: error allocating resp buf\n");
      return false;
    }

    if (piperead(c, resp->buf, resp->lbuf) != resp->lbuf) {
      printf("kproc mount: Error reading resp buf\n");
      return false;
    }
  }
	
  return true;
}

int
mountproc(void *arg)
{
  struct binding *b;
  struct proc *p, *pp;
  struct response *resp;
  bool found;
  char *pathstr;

  b = (struct binding *) arg;

  pathstr = (char *) pathtostr(b->path, nil);

  debug("kproc mount: '/%s' on pid %i\n",
	pathstr, current->pid);
	
  while (b->refs > 0) {
    resp = malloc(sizeof(struct response));
    if (resp == nil) {
      printf("kproc mount: error allocating response.\n");
      break;
    }

    if (!respread(b->in, resp)) {
      printf("kproc mount: error reading responses.\n");
      break;
    }

    lock(&b->lock);

    found = false;
    pp = nil;
    for (p = b->waiting; p != nil; pp = p, p = p->wnext) {
      if (p->waiting.rid == resp->rid) {
	found = true;
	p->aux = (void *) resp;
	procready(p);
	if (pp == nil) {
	  b->waiting = p->wnext;
	} else {
	  pp->wnext = p->wnext;
	}
	break;
      }
    }

    if (!found) {
      printf("%i mounted proc is being bad and replying without a request (%i).\n",
	     current->pid, resp->rid);
      if (resp->lbuf > 0)
	free(resp->buf);
      free(resp);
    }
    
    unlock(&b->lock);
  }

  printf("kproc mount: an error occured with binding '/%s'\n",
	pathstr);
		
  debug("kproc mount: lock binding\n");	
  lock(&b->lock);
  debug("kproc mount: free chans\n");	

  freechan(b->in);
  freechan(b->out);
  debug("kproc mount: unlock binding\n");	
  unlock(&b->lock);

  debug("kproc mount: wait for bindings refs to go to zero.\n");
  while (true) {
    lock(&b->lock);
    if (b->refs == 0)
      break;
    unlock(&b->lock);
    schedule();
  }
	
  debug("kproc mount: no longer bound\n");

  /* Free binding and exit. */

  debug("kproc mount: lock\n");
  lock(&b->lock);	

  debug("kproc mount: wake waiters\n");
	
  /* Wake up any waiting processes so they can error. */
  for (p = b->waiting; p != nil; p = p->wnext) {
    printf("kproc mount: wake up %i\n", p->pid);
    p->aux = nil;
    procready(p);
  }
	
  debug("kproc mount: free path\n");
  freepath(b->path);
  free(b);
	
  printf("kproc mount: '/%s' finished.\n", pathstr);

  free(pathstr);
	
  return 0;
}
