#include "head.h"

struct pipe {
	struct chan *c0, *c1;

	bool waiting;	
	struct proc *proc;
	uint8_t *buf;
	size_t n;
};

bool
newpipe(struct chan **c0, struct chan **c1)
{
	struct pipe *p;
	
	p = malloc(sizeof(struct pipe));
	if (p == nil) {
		return false;
	}
	
	*c0 = newchan(CHAN_pipe, O_RDONLY, nil);
	if (*c0 == nil) {
		free(p);
		return false;
	}
	
	*c1 = newchan(CHAN_pipe, O_WRONLY, nil);
	if (*c1 == nil) {
		free(p);
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

int
piperead(struct chan *c, uint8_t *buf, size_t n)
{
	struct pipe *p;
	
	p = (struct pipe *) c->aux;

	if (p->c0 == nil || p->c1 == nil)
		return ELINK;
		
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

int
pipewrite(struct chan *c, uint8_t *buf, size_t n)
{
	size_t l, t;
	struct pipe *p;
	
	p = (struct pipe *) c->aux;

	if (p->c0 == nil || p->c1 == nil)
		return ELINK;
	
	t = 0;
	while (t < n) {
		while (!p->waiting && p->c0 != nil && p->c1 != nil)
			schedule();
		
		if (p->c0 == nil || p->c1 == nil) {
			return t;
		}
		
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
		}
	}
	
	return t;
}

int
pipeclose(struct chan *c)
{
	struct pipe *p;
	
	p = (struct pipe *) c->aux;
	
	if (c == p->c0) {
		p->c0 = nil;
	} else {
		p->c1 = nil;
	}
	
	if (p->waiting) {
		procready(p->proc);
		p->waiting = false;
	}

	if (p->c0 == nil && p->c1 == nil) {
		free(p);
	}
	
	return 0;
}

struct chantype devpipe = {
	&piperead,
	&pipewrite,
	&pipeclose,
};
