#include "../include/types.h"
#include "../include/std.h"
#include "../port/com.h"

static int
task1_func(void)
{
	puts("task1 started\n");
	while (true) {
		puts("task 1 running\n");
	}
	
	return 1;
}

int
_start(void)
{
	puts("in kmain\n");
	
	if (!fork()) {
		return task1_func();
	}

	puts("tasks initiated\n");

	while (1) {
		yield();
	}

	return 0;
}