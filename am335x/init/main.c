#include "../include/types.h"
#include "../include/std.h"
#include "../include/stdarg.h"
#include "../include/fs.h"

#include "head.h"

/* Must not excede one page in size and all changabel variables 
 * must be on the stack. */

int
main(void)
{
	int f, fds[2];
	int stdout;

	f = fork(FORK_sngroup);
	if (!f) {
		return pmount();
	}
	
	sleep(0);
	
	stdout = open("/dev/com", O_WRONLY);
	if (stdout < 0) {
		return -1;
	}

	printf("Fork frife\n");
	f = fork(FORK_sngroup);
	if (f < 0) {
		printf("fork failed\n");
	} else if (!f) {
		return pfile_open();
	}
	
	sleep(1000);

	if (pipe(fds) == ERR) {
		printf("pipe failed\n");
		return ERR;
	}
	
	sleep(1000);
	
	printf("Fork once\n");
	f = fork(FORK_sngroup);
	if (f < 0) {
		printf("fork failed\n");
	} else if (!f) {
		printf("Close other end of pipe 1\n");
		close(fds[1]);
		return ppipe0(fds[0]);
	}
	
	printf("Close other end of pipe 0\n");
	close(fds[0]);
	
	sleep(1000);
	
	printf("Fork twice\n");
	f = fork(FORK_sngroup);
	if (f < 0) {
		printf("fork failed\n");
	} else if (!f) {
		return ppipe1(fds[1]);
	}
	
	close(fds[1]);

	printf("main task exiting\n");
	
	return 0;
}
