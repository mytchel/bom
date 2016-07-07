#include "dat.h"

struct path *
strtopath(const char *str)
{
	struct path *p;
	int i, j;
	
	for (i = 0; str[i] && str[i] != '/'; i++);
	if (i == 0 && str[i]) {
		return strtopath(str + 1);
	} else if (i == 0) {
		return nil;
	}
	
	p = kmalloc(sizeof(struct path));
	/* Best to cause some more major issue in the case of a failed
	 * kmalloc, as I have no way to check if there was an error
	 * (or end of the path).
	 */
	
	p->refs = 1;
	p->s = kmalloc(sizeof(char) * (i+1));
	for (j = 0; j < i+1; j++)
		p->s[j] = str[j];
	p->s[j] = 0;
	
	p->next = strtopath(str + i);
	return p;
}

char *
pathtostr(struct path *p)
{
	char *str;
	size_t len, i;
	struct path *pp;
	
	len = 0;
	for (pp = p; pp != nil; pp = pp->next)
		len += strlen(pp->s) + 1;

	str = kmalloc(sizeof(char) * (len + 1));
	if (str == nil)
		return nil;
	
	i = 0;
	for (pp = p; pp != nil; pp = pp->next) {
		len = strlen(pp->s);
		str[i++] = '/';
		memmove(&str[i], pp->s, len);
		i += len;
	}
	
	str[i] = 0;
	
	return str;
}

void
freepath(struct path *p)
{
	p->refs--;
	if (p->refs > 0)
		return;
		
	if (p->next)
		kfree(p->next);
	kfree(p->s);
	kfree(p);
}

size_t
pathmatches(struct path *p, struct path *m)
{
	size_t matches = 0;
	struct path *pp, *mp;
	
	pp = p;
	mp = m;
	while (pp != nil && mp != nil) {
		if (strcmp(pp->s, mp->s) == false)
			return matches;
		
		pp = pp->next;
		mp = mp->next;
		matches++;
	}
	
	return matches;
}

size_t
pathelements(struct path *p)
{
	size_t i = 0;
	while (p != nil) {
		p = p->next;
		i++;
	}
	
	return i;
}
