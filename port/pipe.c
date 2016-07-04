#include "dat.h"

struct pipe *
newpipe(void)
{
	struct pipe *p;
	
	p = kmalloc(sizeof(struct pipe));
	if (p == nil) {
		return nil;
	}

	p->refs = 1;
	
	p->action = PIPE_none;
	p->user = nil;
	
	return p;
}

void
freepipe(struct pipe *p)
{
	p->refs--;
	if (p->refs > 0)
		return;
	
	if (p->link != nil)
		p->link->link = nil;
	
	kfree(p);
}

struct path *
strtopath(const char *str)
{
	struct path *p;
	int i, j;
	
	for (i = 0; str[i] && str[i] != '/'; i++);
	if (i == 0 && str[i]) {
		return strtopath(str++);
	} else if (i == 0) {
		return nil;
	}
	
	p = kmalloc(sizeof(struct path));
	/* Best to cause some more major issue in the case of a failed
	 * kmalloc, as I have no way to check if there was an error
	 * (or end of the path).
	 */
	
	p->s = kmalloc(sizeof(char) * i);
	for (j = 0; j < i; j++)
		p->s[j] = str[j];
	p->s[j] = 0;
	
	p->next = strtopath(str + i);
	return p;
}

void
freepath(struct path *p)
{
	if (p->next)
		kfree(p->next);
	kfree(p->s);
	kfree(p);
}
