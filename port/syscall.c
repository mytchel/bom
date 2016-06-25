#include "dat.h"
#include "../port/com.h"
#include "../port/syscall.h"
#include "../include/std.h"

static int
sys_exit(int code)
{
	kprintf("pid %i exited with status %i\n", current->pid, code);
	
	proc_remove(current);
	schedule();
	
	/* Never reached. */
	return 1;
}

static int
sys_fork(void (*func)(void *), void *arg)
{
	struct proc *p;
	p = proc_create(func, arg);
	return p->pid;
}

static int
sys_getpid(void)
{
	return current->pid;
}

static int
sys_mount(struct fs *fs, char *path)
{
	kprintf("mount proc %i at %s\n", current->pid, path);
	
	fs_mount(current, path);
	
	while (1) {
		kprintf("wait for next event\n");

		current->state = PROC_mount;
		reschedule();

		kprintf("run proc func\n");

		run_proc_func(fs->open);

		kprintf("back in mounted process %i\n", current->pid);
		
	}
	
	return 0;
}

static int
sys_open(const char *path, int mode)
{
	struct proc *m;

	m = fs_find_mount(current, path);
	if (m == nil) {
		return -1;
	}
	
	while (m->state != PROC_mount) {
		reschedule();
	}

	kprintf("Set message to mount process\n");
	m->state = PROC_ready;
	current->state = PROC_wait;
	reschedule();

	kprintf("lets just pretend there is inter process communication for now\n");
	
	return -1;
}

static int
sys_close(int fd)
{
	kprintf("should close '%i'\n", fd);
	
	current->state = PROC_wait;
	reschedule();
	
	return -1;
}

static int
sys_stat(int fd)
{
	kprintf("should stat '%i'\n", fd);
	
	current->state = PROC_wait;
	reschedule();
	
	return -1;
}

static int
sys_read(int fd, char *buf, size_t n)
{
	kprintf("should read but instead rescheduling %i\n", current->pid);
	
	
	current->state = PROC_wait;
	reschedule();
	
	kprintf("and back in kernel for %i\n", current->pid);
	
	return -1;
}

static int
sys_write(int fd, char *buf, size_t n)
{
	kprintf("should write\n");

	current->state = PROC_wait;
	reschedule();

	return -1;
}

reg_t syscall_table[] = {
	[SYSCALL_EXIT] 		= (reg_t) sys_exit,
	[SYSCALL_FORK] 		= (reg_t) sys_fork,
	[SYSCALL_GETPID]	= (reg_t) sys_getpid,
	[SYSCALL_MOUNT] 	= (reg_t) sys_mount,
	[SYSCALL_OPEN]		= (reg_t) sys_open,
	[SYSCALL_CLOSE]		= (reg_t) sys_close,
	[SYSCALL_STAT]		= (reg_t) sys_stat,
	[SYSCALL_READ]		= (reg_t) sys_read,
	[SYSCALL_WRITE]		= (reg_t) sys_write,
};

