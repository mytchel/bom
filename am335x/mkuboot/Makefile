#	$OpenBSD: Makefile,v 1.3 2013/10/28 09:00:06 patrick Exp $

am335x/mkuboot/mkuboot: am335x/mkuboot/mkuboot.c am335x/mkuboot/copy_elf32.c am335x/mkuboot/copy_elf64.c
	cc -o out/mkuboot am335x/mkuboot/mkuboot.c am335x/mkuboot/copy_elf32.c am335x/mkuboot/copy_elf64.c -lz
