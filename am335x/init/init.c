#include "../include/types.h"
#include "../include/std.h"
#include "../port/com.h"

static int
task1(void)
{
	puts("task 1 started\n");
	while (true) {
		puts("task 1 running\n");
		sleep(5000);
	}
	
	return 1;
}

static int
task2(void)
{
	puts("task 2 started\n");
	while (true) {
		puts("task 2 running\n");
		sleep(1000);
	}
	
	return 2;
}

static int
task3(void)
{
	puts("task 3 started\n");
	while (true);
	return 3;
}

int
main(void)
{
	puts("Hello from true user space!\n");
	
	sleep(1000);
	
	puts("Fork once\n");
	if (!fork()) {
		return task1();
	}
	
	puts("Fork twice\n");
	if (!fork()) {
		return task2();
	}
	
	puts("Fork thrice\n");
	if (!fork()) {
		return task3();
	}

	puts("tasks initiated\n");

	return 0;
}
