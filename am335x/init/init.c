#include "../include/types.h"
#include "../include/std.h"
#include "../include/stdarg.h"

/* Must not excede one page in size and all changabel variables 
 * must be on the stack. */

char
getc(void);

void
puts(const char *);

void
printf(const char *, ...);

int
task1(int fd)
{
	int i, pid = getpid();
	
	printf("%i: task 1 started with fd %i\n", pid, fd);
	while (true) {
		if (read(fd, (void *) &i, sizeof(int)) != sizeof(int)) {
			printf("%i: failed to read\n", pid);
			break;
		}
		
		printf("%i: read %i\n", pid, i);
	}
	
	printf("%i: exiting\n", pid);

	return 1;
}

int
task2(int fd)
{
	int i, pid = getpid();
	printf("%i: task 2 started with fd %i\n", pid, fd);
	
	sleep(2000);
	
	for (i = 0; i < 4; i++) {
		sleep(500);
		
		printf("%i: writing %i\n", pid, i);

		if (write(fd, (void *) &i, sizeof(int)) != sizeof(int)) {
			printf("%i: write failed\n", pid);
			break;
		}
	}
	
	printf("%i: exiting\n", pid);

	return 2;
}

int
main(void)
{
	int f, fds[2];

	printf("Hello from true user space!\n");
	
	if (pipe(fds) == -1) {
		printf("pipe failed\n");
		return ERR;
	}
	
	sleep(1000);
	
	printf("Fork once\n");
	f = fork(FORK_cfgroup | FORK_cmem);
	if (!f) {
		close(fds[1]);
		return task1(fds[0]);
	}
	
	close(fds[0]);
	
	sleep(1000);
	
	printf("Fork twice\n");
	f = fork(FORK_cfgroup | FORK_cmem);
	if (!f) {
		return task2(fds[1]);
	}
	
	close(fds[1]);
	
	sleep(1000);

	printf("main task exiting\n");
	
	return 0;
}
