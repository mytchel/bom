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
	int i, n, pid = getpid();
	
	printf("%i: task 1 started with fd %i\n", pid, fd);
	while (true) {
		printf("%i: wait for write\n", pid);
		n = read(fd, (void *) &i, sizeof(int));
		if (n != sizeof(int)) {
			printf("%i: failed to read. %i\n", pid, n);
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
	int i, n, pid = getpid();
	printf("%i: task 2 started with fd %i\n", pid, fd);
	
	for (i = 0; i < 4; i++) {
		printf("%i: sleep\n", pid);
		
		sleep(2000);
		
		printf("%i: writing %i\n", pid, i);

		n = write(fd, (void *) &i, sizeof(int));
		if (n != sizeof(int)) {
			printf("%i: write failed. %i\n", pid, n);
			break;
		}
	}
	
	printf("%i: exiting\n", pid);

	return 2;
}

int
task3(void)
{
	int pid = getpid();
	int in, out;
	int p1[2], p2[2];
	
	printf("%i: set up pipes\n", pid);
	
        if (pipe(p1) == ERR) {
        	printf("%i: p1 pipe failed\n", pid);
        	return -3;
        } else if (pipe(p2) == ERR) {
        	printf("%i: p2 pipe failed\n", pid);
        	return -3;
        }
	
	printf("%i: bind\n", pid);

	if (bind(p1[1], p2[0], "/") < 0) {
		printf("%i: bind failed\n", pid);
		return -3;
	}

	printf("%i: close p1[1]\n", pid);
	close(p1[1]);
	printf("%i: close p2[0]\n", pid);	
	close(p2[0]);
	
	in = p1[0];
	out = p2[1];

	char buf[3];	
	while (true) {
		printf("%i: read requests\n", pid);
		
		if (read(in, buf, sizeof(char) * 3) < 0) {
			printf("%i: failed to read in\n", pid);
			break;
		}
		
		printf("%i: read: '%s'\n", pid, buf);
		
		write(out, buf, sizeof(char) * 3);
	}

	return 3;
}

int
task4(void)
{
	int pid = getpid();
	int fd;
	
	fd = open("/com", O_WRONLY);
	if (fd == -1) {
		printf("%i: open /com failed\n", pid);
		return -4;
	}
	
	while (true) {
		write(fd, "Hello\n", sizeof(char) * 7);
		sleep(5000);
	}
	
	return 4;
}

int
main(void)
{
	int f, fds[2];

	printf("Hello from true user space!\n");
	
	if (pipe(fds) == ERR) {
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
	
	f = fork(FORK_cfgroup | FORK_cmem);
	if (!f) {
		return task3();
	}
	
	sleep(1000);

	f = fork(FORK_cfgroup | FORK_cmem);
	if (!f) {
		return task4();
	}
	
	sleep(1000);

	printf("main task exiting\n");
	
	return 0;
}
