#include "../include/types.h"
#include "../include/std.h"
#include "../include/stdarg.h"

/* Make sure that the compiled length does 
 * not excede one page (4096) */

void
puts(const char *);

void
kprintf(const char *, ...);

int
task1(int fd)
{
	int i;
	kprintf("task 1 started with fd %i\n", fd);
	while (true) {
		puts("task 1 read an int\n");
		if (read(fd, (void *) &i, sizeof(int)) == -1) {
			puts("task 1 read failed\n");
		} else {
			kprintf("task 1 read %i\n", i);
		}
	}
	
	return 1;
}

int
task2(int fd)
{
	int i = 0;
	kprintf("task 2 started with fd %i\n", fd);
	while (true) {
		kprintf("task 2 writing %i\n", i);
		if (write(fd, (void *) &i, sizeof(int)) == -1) {
			puts("task 2 write failed\n");
		} else {
			puts("task 2 wrote\n");
		}
		
		i++;

		puts("task 2 sleeping\n");		
		sleep(5000);
	}
	
	return 2;
}

int
task3(void)
{
	puts("task 3 started\nMay well crash\n");
	char msg[] = "Hello there, this is a test.";
	kprintf("msg = '%s'\n", msg);
	return 3;
}

int
main(void)
{
	int f, fds[2];

	puts("Hello from true user space!\n");
	
	if (pipe(fds) == -1) {
		puts("pipe failed\n");
		exit(-1);
	}
	kprintf("fd[0] = %i, fd[1] = %i\n", fds[0], fds[1]);
	
	sleep(1000);
	
	puts("Fork once\n");
	f = fork(0);
	if (!f) {
		return task1(fds[1]);
	}
	
	puts("Fork twice\n");
	f = fork(0);
	if (!f) {
		return task2(fds[0]);
	}
	
	puts("Fork thrice\n");
	f = fork(0);
	if (!f) {
		return task3();
	}

	puts("tasks initiated\n");

	return 0;
}
