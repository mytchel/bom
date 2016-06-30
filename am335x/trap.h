#ifndef __TRAP_H
#define __TRAP_H

#define MODE_SVC		19
#define MODE_FIQ		17
#define MODE_IRQ		18
#define MODE_ABORT		23
#define MODE_UND		27
#define MODE_SYS		31
#define MODE_USR		16

#define ABORT_INTERRUPT		0
#define ABORT_INSTRUCTION	1
#define ABORT_PREFETCH		2
#define ABORT_DATA	 	3

#endif
