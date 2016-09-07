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
      printf("kproc mount : error allocating resp buf\n");
      return false;
    }

    if (piperead(c, resp->buf, resp->lbuf) != resp->lbuf) {
      printf("kproc mount : Error reading resp buf\n");
      return false;
    }
  }
	
  return true;
}

int
mountproc(void *arg)
{
  struct binding *b = arg;
  struct proc *p, *pp;
  struct response *resp;
  bool found;
	
  while (b->refs > 0) {
    resp = malloc(sizeof(struct response));
    if (resp == nil) {
      printf("kproc mount : error allocating response.\n");
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
      printf("%i mounted proc is being bad.\n", current->pid);
      if (resp->lbuf > 0)
	free(resp->buf);
      free(resp);
    }
    
    unlock(&b->lock);
  }
	
  printf("kproc mount : an error occured\n");
  printf("free chans\n");	
  lock(&b->lock);
  freechan(b->in);
  freechan(b->out);
  unlock(&b->lock);
	
  while (true) {
    lock(&b->lock);
    if (b->refs == 0)
      break;
    unlock(&b->lock);
    schedule();
  }
	
  printf("No longer bound\n");

  /* Free binding and exit. */

  printf("wake waiters\n");
	
  /* Wake up any waiting processes so they can error. */
  for (p = b->waiting; p != nil; p = p->wnext) {
    p->aux = nil;
    procready(p);
  }
	
  printf("lock\n");
  lock(&b->lock);	

  printf("free path\n");
  freepath(b->path);
	
  free(b);
	
  printf("return\n");
	
  return 0;
}
