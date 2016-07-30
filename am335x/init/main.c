#include "head.h"

/* Must not excede one page in size and all changabel variables 
 * must be on the stack. */

int stdin, stdout, stderr;

int
main(void)
{
	int f, fds[2];

	f = fork(FORK_sngroup);
	if (!f) {
		return pmount();
	}
	
	sleep(10);

	stdin = open("/dev/com", O_RDONLY);
	stdout = open("/dev/com", O_WRONLY);
	stderr = open("/dev/com", O_WRONLY);
	if (stdin < 0 || stdout < 0 || stderr < 0)
		return -1;
	
	f = fork(FORK_sngroup);
	if (f < 0) {
		return -1;
	} else if (!f) {
		return pfile_open();
	}

	if (pipe(fds) == ERR) {
		return -1;
	}

	sleep(1000);
	f = fork(FORK_sngroup);
	if (f < 0) {
		return -2;
	} else if (!f) {
		close(fds[1]);
		return ppipe0(fds[0]);
	}
	
	close(fds[0]);
	
	sleep(1000);
	f = fork(FORK_sngroup);
	if (f < 0) {
		return -3;
	} else if (!f) {
		return ppipe1(fds[1]);
	}
	
	close(fds[1]);

	sleep(1000);
	return 0;
}
