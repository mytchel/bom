#include "types.h"
#include "../include/com.h"
#include "../include/proc.h"

static void
task1_func(void *arg)
{
	int i = 0, j;
	kprintf("task1 started\n");
	while (1) {
		for (j = 0; j < 100000; j++);
		kprintf("task 1 i = %i\n", i++);
	}
}

static void
task2_func(void *arg)
{
	int i = 0;
	
	kprintf("task2 started\n");
	while (1) {
		kprintf("task2 %i\n", i++);
	}
}

static void
task3_func(void *arg)
{
	int i = 0;
	
	kprintf("task3 started\n");
	while (1) {
		kprintf("task3 %i\n", i++);
	}
}


void
kmain(void *arg)
{
	kprintf("in kmain\n");
	
	kprintf("adding tasks\n");
	
	proc_create(&task1_func, nil);
	proc_create(&task2_func, nil);
	proc_create(&task3_func, nil);

	kprintf("tasks initiated\n");
	
	while (1) {
		int j;
		for (j = 0; j < 10000; j++);
		kprintf("doing nothing\n");
	}
}
