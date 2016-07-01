#include "../include/types.h"
#include "../include/std.h"

/* Make sure that the compiled length does 
 * not excede one page (4096) */

void
puts(const char *);

void
kprintf(const char *, ...);

int
task1(void)
{
	puts("task 1 started\n");
	while (true) {
		puts("task 1 running\n");
		sleep(5000);
	}
	
	return 1;
}

int
task2(void)
{
	int i = 0;
	puts("task 2 started\n");
	while (true) {
		kprintf("task 2 running: %i\n", i++);
		sleep(1000);
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
	int f;
	puts("Hello from true user space!\n");
	
	sleep(1000);
	
	puts("Fork once\n");
	f = fork();
	if (!f) {
		return task1();
	}
	
	kprintf("Forked %i\n", f);
	puts("Fork twice\n");
	f = fork();
	if (!f) {
		return task2();
	}
	
	kprintf("Forked %i\n", f);
	puts("Fork thrice\n");
	f = fork();
	if (!f) {
		return task3();
	}

	kprintf("Forked %i\n", f);
	puts("tasks initiated\n");

	return 0;
}
