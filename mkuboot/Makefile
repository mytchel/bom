#	$OpenBSD: Makefile,v 1.3 2013/10/28 09:00:06 patrick Exp $

SRCS=	mkuboot.c copy_elf32.c copy_elf64.c
LDADD=	-lz
CFLAGS=	-DMACHINE_ARCH=\"${MACHINE_ARCH}\"

mkuboot: ${SRCS}
	${CC} -o mkuboot ${SRCS} ${CFLAGS} ${LDADD}
	
clean:
	rm mkuboot
	 