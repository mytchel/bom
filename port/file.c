#include "dat.h"

int
mountproc(void *arg)
{
	struct binding *b = arg;
	struct proc *p, *pp;
	
	kprintf("New kernel mount proc started\n");
	
	while (b->refs > 0) {
		while (b->resp != nil)
			schedule();
		
		b->resp = kmalloc(sizeof(struct response));
		if (b->resp == nil) {
			kprintf("kproc mount : error allocating response.\n");
			break;
		}
		
		if (piperead(b->in, (uint8_t *) b->resp, sizeof(struct response)) <= 0) {
			kprintf("kproc mount : Error reading pipe\n");
			break;
		}
		
		if (b->resp->n > 0) {
			b->resp->buf = kmalloc(b->resp->n);
			if (b->resp->buf == nil) {
				kprintf("kproc mount : error allocating resp buf\n");
				break;
			}
			
			if (piperead(b->in, b->resp->buf, b->resp->n) != b->resp->n) {
				kprintf("kproc mount : Error reading resp buf\n");
				break;
			}
		}
		
		lock(&b->lock);
		
		pp = nil;
		for (p = b->waiting; p != nil; pp = p, p = p->wnext) {
			if (p->fid == b->resp->fid) {
				procready(p);
				if (pp == nil) {
					b->waiting = p->wnext;
				} else {
					pp->wnext = p->wnext;
				}
				p->wnext = nil;
				break;
			}
		}
		
		unlock(&b->lock);
		schedule();
	}
	
	kprintf("No longer bound\n");

	/* Free binding and exit. */

	kprintf("wake waiters\n");
		
	/* Wake up any waiting processes so they can error. */
	for (p = b->waiting; p != nil; p = p->wnext) {
		procready(p);
	}
	
	kprintf("lock\n");
	lock(&b->lock);	

	kprintf("free chans\n");	
	freechan(b->in);
	freechan(b->out);
	
	kprintf("free path\n");
	freepath(b->path);
	
	if (b->resp) {
		kprintf("bind free'd with response\n");
		kfree(b->resp);
	}
	
	kfree(b);
	
	kprintf("return\n");
	
	return 0;
}

static struct response *
makereq(struct binding *b, struct request *req)
{
	struct response *resp;
	
	lock(&b->lock);

	kprintf("write request\n");
	
	req->fid = b->nreqid++;
	pipewrite(b->out, (uint8_t *) req, sizeof(struct request));
	
	if (req->n > 0) {
		pipewrite(b->out, req->buf, req->n);
	}
	
	current->fid = req->fid;
	current->wnext = b->waiting;
	b->waiting = current;
	
	unlock(&b->lock);
	
	procwait(current);
	schedule();

	lock(&b->lock);
	resp = b->resp;	
	b->resp = nil;
	unlock(&b->lock);
	
	return resp;
}

struct chan *
fileopen(struct binding *b, struct path *path, int flag, int mode, int *err)
{
	struct request *req;
	struct response *resp;
	
	*err = ERR;
	kprintf("file open\n");
	
	req = kmalloc(sizeof(struct request));
	if (req == nil)
		return nil;
	
	req->type = REQ_open;
	req->buf = (uint8_t *) pathtostr(path, &(req->n));
	
	kprintf("req path = '%s'\n", req->buf);
	
	resp = makereq(b, req);
	
	*err = resp->err;

	kfree(resp);
	return nil;
}

int
fileread(struct chan *c, uint8_t *buf, size_t n)
{
	return ERR;
}

int
filewrite(struct chan *c, uint8_t *buf, size_t n)
{
	return ERR;
}

int
fileclose(struct chan *c)
{
	return ERR;
}

struct chantype devfile = {
	&fileread,
	&filewrite,
	&fileclose,
};
