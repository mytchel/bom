#include "dat.h"
#include "../port/com.h"
#include "../port/syscall.h"
#include "../include/std.h"

static void
task1_func(void *arg)
{
	uint32_t i = 0, j;
	for (i = 0; i < 100000; i++) {
		for (j = 0; j < 0xffffffff; j++);
		if (i % 1000 == 0)
			kprintf("task 1 i = %i\n", i);
	}
	
	kprintf("task1 finished\n");
	exit(0);
}

static void
task2_func(void *arg)
{
	puts("task2 started\n");
	uint32_t i;
	for (i = 0; i < 1000; i++) {
		puts("fdjkaslfjdskal;\n");
	}
	exit(1);
}

static void
task3_func(void *arg)
{
	uint32_t i, j;
	for (i = 0; i < 100000; i++) {
		for (j = 0; j < 0x1fffffff; j++);
		if (i % 3000 == 0)
			kprintf("task3 %i\n", i);
	}
	
	exit(2);
}

static void
task4_func(void *arg)
{
	uint32_t i, j, w;
	reg_t *addr = (reg_t *) 1;
	
	kprintf("task4 started with pid %i\n", getpid());
	
	for (i = 0; i < 10; i++) {
		for (j = 0; j < 0xffffffff; j++);
		kprintf("accessing 0x%h\n", addr);
		w = (int) (*addr);
		kprintf("got 0x%h\n", w);
		
		addr += 100;
	}
	
	exit(3);
}


void
kmain(void *arg)
{
	kprintf("in kmain\n");
	
	kprintf("adding tasks\n");
	
	proc_create(&task1_func, nil);
	proc_create(&task2_func, nil);
	proc_create(&task3_func, nil);
	proc_create(&task4_func, nil);

	kprintf("tasks initiated\nmain exiting...\n");
	
	exit(0);
}
