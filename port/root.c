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

struct binding *rootbinding;

static int
rootproc(void *arg);

void
initroot(void)
{

  struct chan *c[4];
  struct proc *pr, *ps;

  if (!newpipe(&c[0], &c[1])) {
    panic("initroot: newpipe failed!\n");
  } else if (!newpipe(&c[2], &c[3])) {
    panic("initroot: newpipe failed!\n");
  }
  
  rootbinding = newbinding(nil, c[1], c[2]);
  if (rootbinding == nil) {
    panic("initroot: newbinding failed!\n");
  }
		
  pr = newproc();
  if (pr == nil) {
    panic("initroot: newproc failed!\n");
  }

  initproc(pr);
  forkfunc(pr, &rootproc, (void *) c);

  pr->fgroup = newfgroup();
  pr->ngroup = nil;
  pr->parent = nil;
	
  ps = newproc();
  if (ps == nil) {
    panic("initroot: newproc failed!\n");
  }

  initproc(ps);
  forkfunc(ps, &mountproc, (void *) rootbinding);
	
  rootbinding->srv = ps;
	
  procready(pr);
  procready(ps);
}

static void
bopen(struct request *req, struct response *resp)
{
  printf("root open\n");
}

static void
bclose(struct request *req, struct response *resp)
{
  printf("root close\n");
}

static void
bwalk(struct request *req, struct response *resp)
{
  printf("root walk\n");
}

static void
bread(struct request *req, struct response *resp)
{
  printf("root read\n");
  resp->ret = ENOIMPL;
}

static void
bwrite(struct request *req, struct response *resp)
{
  printf("root write\n");
  resp->ret = ENOIMPL;
}

static void
bremove(struct request *req, struct response *resp)
{
  printf("root remove\n");
}

static void
bcreate(struct request *req, struct response *resp)
{
  printf("root create\n");
}

static struct fsmount mount = {
  bopen,
  bclose,
  bwalk,
  bread,
  bwrite,
  bremove,
  bcreate
};

static int
rootproc(void *arg)
{
  struct chan **c;
  int in, out;

  c = (struct chan **) arg;
  in = addchan(current->fgroup, c[0]);
  out = addchan(current->fgroup, c[3]);

  return fsmountloop(in, out, &mount);
}
