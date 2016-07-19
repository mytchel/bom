#include "dat.h"

struct pipe {
	struct chan *c0, *c1;

	bool waiting;	
	struct proc *proc;
	char *buf;
	size_t n;
};

bool
newpipe(struct chan **c0, struct chan **c1)
{
	struct pipe *p;
	
	p = kmalloc(sizeof(struct pipe));
	if (p == nil) {
		return false;
	}
	
	*c0 = newchan(CHAN_pipe, O_RDONLY, nil);
	if (*c0 == nil) {
		kfree(p);
		return false;
	}
	
	*c1 = newchan(CHAN_pipe, O_WRONLY, nil);
	if (*c1 == nil) {
		kfree(p);
		freechan(*c0);
		return false;
	}
	
	(*c0)->aux = p;
	(*c1)->aux = p;
	
	p->c0 = *c0;
	p->c1 = *c1;
	p->waiting = false;
	
	return true;	
}

static int
piperead(struct chan *c, char *buf, size_t n)
{
	struct pipe *p;
	
	p = (struct pipe *) c->aux;
	
	p->waiting = true;
	p->proc = current;
	p->buf = buf;
	p->n = n;
	
	procwait(current);
	schedule();
	
	if (p->n != 0) {
		return n - p->n;
	} else {
		return n;
	}
}

static int
pipewrite(struct chan *c, char *buf, size_t n)
{
	size_t l, t;
	struct pipe *p;
	
	p = (struct pipe *) c->aux;
	
	t = 0;
	while (t < n) {
		while (!p->waiting)
			schedule();
		
		l = n - t < p->n ? n - t : p->n;
		memmove(p->buf, buf, l);
		
		t += l;
		buf += l;
		
		if (l < p->n) {
			p->n -= l;
			p->buf += l;
		} else {
			p->n = 0;
			p->waiting = false;
			procready(p->proc);
			schedule();
		}
	}
	
	return n;
}

static int
pipeclose(struct chan *c)
{
	kprintf("Close pipe\n");
	return ERR;
}

struct chantype devpipe = {
	&piperead,
	&pipewrite,
	&pipeclose,
};
