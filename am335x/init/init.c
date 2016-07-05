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
	char c;
	int pid = getpid();
	
	printf("%i: started with fd %i\n", pid, fd);
	while (true) {
		printf("%i: read from pipe\n", pid);
		if (read(fd, (void *) &c, sizeof(char)) <= 0) {
			printf("%i: failed to read\n", pid);
			break;
		}
		
		printf("%i: read %i\n", pid, c);
	}
	
	printf("%i: exiting\n", pid);

	return 1;
}

int
task2(int fd)
{
	char c;
	int pid = getpid();
	printf("%i: started with fd %i\n", pid, fd);
	
	while (true) {
		printf("%i: Press a key\n", pid);
		c = getc();
		printf("%i: %c\n", pid, c);

		if (write(fd, (void *) &c, sizeof(char)) <= 0) {
			printf("%i: write failed\n", pid);
			break;
		}
		
		sleep(1000);
	}
	
	printf("%i: exiting\n", pid);

	return 2;
}

int
task3(void)
{
	char data[10];
	int fd;
	int pid = getpid();
	
	printf("%i: started\n", pid);
	
	fd = open("test", O_RDONLY);
	if (fd == -1) {
		printf("open failed\n");
		return 3;
	}
	
	printf("%i: fd = %i\n", pid, fd);
	
	if (read(fd, data, sizeof(data)) > 0) {
		data[9] = 0; /* to be sure. */
		printf("%i: read: '%s'\n", pid, data);
	} else {
		printf("%i: read failed.\n", pid);
	}
	
	close(fd);
	
	return 3;
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
		close(fds[0]);
		return task1(fds[1]);
	}
	
	sleep(1000);
	
	printf("Fork twice\n");
	f = fork(FORK_cfgroup | FORK_cmem);
	if (!f) {
		close(fds[1]);
		return task2(fds[0]);
	}
	
	sleep(1000);
	
	printf("Fork thrice\n");
	f = fork(FORK_cfgroup | FORK_cmem);
	if (!f) {
		close(fds[0]);
		close(fds[1]);
		return task3();
	}
	
	printf("tasks initiated\n");

	sleep(1000);

	printf("main task exiting\n");
	
	return 0;
}
