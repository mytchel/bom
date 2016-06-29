#include "../include/types.h"
#include "../include/std.h"
#include "../port/com.h"

static int
task1(void)
{
	puts("task1 started\n");
	while (true) {
		puts("task 1 running\n");
	}
	
	return 1;
}

static int
task2(void)
{
	puts("task2 started\n");
	while (true) {
		puts("task 2 running\n");
	}
	
	return 2;
}

int
main(void)
{
	puts("Hello from true user space!\n");
	
	if (!fork()) {
		return task1();
	}

	if (!fork()) {
		return task2();
	}

	puts("tasks initiated\n");

	while (1) {
		yield();
	}

	return 0;
}
