#include "dat.h"
#include "../port/com.h"
#include "../include/std.h"

static void fs_proc(void *arg);

struct mount {
	struct proc *proc;
	char path[256];
	struct mount *next;
};

struct mount *root;

void
fs_init(void)
{
	kprintf("fs_init\n");
	
	root = nil;
	
	proc_create(&fs_proc, nil);
}

void
fs_mount(struct proc *p, char *path)
{
	int i;
	struct mount *m;

	if (root == nil) {
		root = kmalloc(sizeof(struct mount));
		m = root;
	} else {
		for (m = root; m->next; m = m->next);
		m->next = kmalloc(sizeof(struct mount));
		m = m ->next;
	}
	
	if (m == nil) {
		panic("failed to mount\n");
	}
	
	m->proc = p;
	m->next = nil;
	
	for (i = 0; path[i]; i++)
		m->path[i] = path[i];
	m->path[i] = 0;
}

struct proc *
fs_find_mount(struct proc *p, const char *path)
{
	struct mount *m;
	int i;
	
	kprintf("find mount owner for '%s' for proc %i\n", path, p->pid);
	
	for (m = root; m != nil; m = m->next) {
		kprintf("check '%s'\n", m->path);
		for (i = 0; m->path[i] && path[i]; i++)
			if (m->path[i] != path[i])
				break;

		if (m->path[i] == 0 && path[i] == 0) {
			kprintf("found\n");
			return m->proc;
		}
	}
	
	return nil;
}

static int
fs_proc_open(const char *path, int mode)
{
	kprintf("/ open '%s' with mode %i\n", path, mode);
	return -1;
}

static int
fs_proc_close(int fd)
{
	return -1;
}

static int
fs_proc_stat(int fd)
{
	return -1;
}

static int
fs_proc_write(int fd, char *buf, size_t size)
{
	return -1;
}

static int
fs_proc_read(int fd, char *buf, size_t size)
{
	return -1;
}

static void
fs_proc(void *arg)
{
	kprintf("In fs_proc\n");

	struct fs fs = {
		&fs_proc_open,
		&fs_proc_close,
		&fs_proc_stat,
		&fs_proc_write,
		&fs_proc_read,
	};

	mount(&fs, "/");	
}

