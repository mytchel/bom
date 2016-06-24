#include "../port/com.h"
#include "../include/types.h"
#include "../include/std.h"

static void
task1_func(void *arg)
{
	int i = 0, j;
	for (i = 0; i < 100000; i++) {
		for (j = 0; j < 0x1fffffff; j++);
		if (i % 1000 == 0)
			kprintf("task 1 i = %i\n", i);
	}
	
	kprintf("task1 finished\n");
	exit(0);
}

static void
task2_func(void *arg)
{
	int i, fd;
	puts("task2 started\n");
	for (i = 0; i < 3; i++) {
		kprintf("open /\n");
		fd = open("/", 0);
		puts("ok\n");
		if (fd == -1) {
			kprintf("open failed\n");
		} else {
			kprintf("opened file %i\n", fd);
			close(fd);
		}
	}
	
	kprintf("task2 exiting\n");
	exit(1);
}

static void
task3_func(void *arg)
{
	int i;
	for (i = 0; i < 100000; i++) {
		if (i % 3000 == 0)
			kprintf("task3 %i\n", i);
	}

	kprintf("task3 exiting\n");	
	exit(2);
}

static void
task4_func(void *arg)
{
	int i, j, w;
	int *addr = (int *) 1;
	
	kprintf("task4 started with pid %i\n", getpid());
	
	for (i = 0; i < 10; i++) {
		for (j = 0; j < 0x1fffffff; j++);
		kprintf("accessing 0x%h\n", addr);
		w = (int) (*addr);
		kprintf("got 0x%h\n", w);
		
		addr += 100;
	}

	kprintf("task4 exiting\n");	
	exit(3);
}


void
kmain(void *arg)
{
	kprintf("in kmain\n");
	
	kprintf("adding tasks\n");
	
	fork(&task1_func, nil);
	fork(&task2_func, nil);
	fork(&task3_func, nil);
	fork(&task4_func, nil);

	kprintf("tasks initiated\nmain exiting...\n");
	
	exit(0);
}
