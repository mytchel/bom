#include "../include/types.h"
#include "../include/std.h"
#include "../include/stdarg.h"

/* Make sure that the compiled length does 
 * not excede one page (4096) */

char
getc(void);

void
puts(const char *);

void
kprintf(const char *, ...);

int
task1(int fd)
{
	char c;
	kprintf("task 1 started with fd %i and pid %i\n", fd, getpid());
	while (true) {
		if (read(fd, (void *) &c, sizeof(char)) == -1) {
			puts("task 1 read failed\n");
		} else {
			kprintf("task 1 read %i\n", c);
		}
	}
	
	return 1;
}

int
task2(int fd)
{
	char c;
	kprintf("task 2 started with fd %i and pid %i\n", fd, getpid());
	while (true) {
		puts("Press a key: ");
		c = getc();
		kprintf("%c\n", c);
		if (write(fd, (void *) &c, sizeof(char)) == -1) {
			puts("task 2 write failed\n");
		}
		
		sleep(0); /* yield so task 1 can process input */;
	}
	
	return 2;
}

int
task3(void)
{
	puts("task 3 started\n");
	char msg[] = "Hello there, this is a test.";
	kprintf("msg: '%s'\n", msg);
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
	
	sleep(1000);
	
	puts("Fork once\n");
	f = fork(0);
	if (!f)
		return task1(fds[1]);
	
	puts("Fork twice\n");
	f = fork(0);
	if (!f)
		return task2(fds[0]);
	
	puts("Fork thrice\n");
	f = fork(0);
	if (!f)
		return task3();

	puts("tasks initiated\n");

	return 0;
}
