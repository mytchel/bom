#
#  Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
#  
#  Permission is hereby granted, free of charge, to any person
#  obtaining a copy of this software and associated documentation
#  files (the "Software"), to deal in the Software without
#  restriction, including without limitation the rights to use,
#  copy, modify, merge, publish, distribute, sublicense, and/or
#  sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following
#  conditions:
#  
#  The above copyright notice and this permission notice shall be
#  included in all copies or substantial portions of the Software.
#  
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
#  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#  OTHER DEALINGS IN THE SOFTWARE
#

.SUFFIXES: .S .c .h .o .elf .bin .list .umg

.PHONY: all
all: am335x.list init.list am335x.umg copy

CLEAN = 
DEBUG ?= 0

loadaddr=0x82000000

HOSTCC ?= cc
CROSS_COMPILE ?= arm-none-eabi-

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

CLEAN += am335x.*

CFLAGS :=-mcpu=cortex-a8 \
        -Wall -Werror -O2 \
        -nostdinc -ffreestanding \
        -std=c89 \
        -D_AM335X_ -DDEBUG=${DEBUG} \
        -I${.CURDIR}/../include -I${.CURDIR}

LDFLAGS := -nostdlib -nodefaultlibs
LGCC := -L/usr/local/lib/gcc/arm-none-eabi/4.9.3/ -lgcc

HOST_CFLAGS := -Wall -Werror -std=c89
HOST_LDFLAGS :=

include ../tools/mkuboot/local.mk
include ../tools/bintoarray/local.mk

ISRC_A := \
	init/syscalls.S		\


ISRC_C := \
	init/com.c              \
	init/mmc/mmc.c          \
        init/mmc/misc.c


ISRC_P := \
        ../init/main.c          \
        ../init/printf.c        \
        ../init/heap.c          \
        ../init/tmp.c           \
        ../init/mbr.c           \
        ../init/shell.c         \
        ../lib/fssrv.c          \
        ../lib/misc.c           \
        ../lib/string.c         \
        ../lib/ctype.c


IOBJECTS := \
	$(ISRC_A:%.S=%.o) \
	$(ISRC_C:%.c=%.o) \
	$(ISRC_P:%.c=%.o)


CLEAN += $(IOBJECTS)


KSRC_A := \
        asm.S                   \
        vectors.S


KSRC_C := \
        initcode.c              \
        main.c                  \
        trap.c                  \
        uart.c                  \
        timer.c                 \
        mem.c                   \
        proc.c                  \
        syscall.c               \
        mmu.c


KSRC_P := \
        ../port/com.c           \
        ../port/proc.c          \
        ../port/lock.c          \
        ../port/page.c          \
        ../port/fgroup.c        \
        ../port/ngroup.c        \
        ../port/chan.c          \
        ../port/pipe.c          \
        ../port/file.c          \
        ../port/mount.c         \
        ../port/fs.c            \
        ../port/rootfs.c        \
        ../port/procfs.c        \
        ../port/path.c          \
        ../port/heap.c          \
        ../port/sysproc.c       \
        ../port/sysfile.c       \
        ../port/syscall.c       \
        ../lib/string.c         \
	../lib/misc.c           \
        ../lib/ctype.c


KOBJECTS := \
	$(KSRC_A:%.S=%.o) \
	$(KSRC_C:%.c=%.o) \
	$(KSRC_P:%.c=%.o)


CLEAN += $(KOBJECTS)


.c.o .S.o:
	@echo CC $<
	@$(CC) $(CFLAGS) -c -o $@ $<


.elf.bin:
	@echo OBJCOPY $@
	@$(OBJCOPY) -Obinary $< $@


.elf.list:
	@echo OBJDUMP $@
	@$(OBJDUMP) -S $< > $@


CLEAN += am335x.elf am335x.list
am335x.elf: kernel.ld $(KOBJECTS)
	@echo LD $@
	@$(LD) $(LDFLAGS) -T kernel.ld -o $@ \
		$(KOBJECTS) \
		$(LGCC)


CLEAN += init.elf init.list
init.elf: init.ld $(IOBJECTS)
	@echo LD $@
	@$(LD) $(LDFLAGS) -T init.ld -o $@ \
		$(IOBJECTS) \
		$(LGCC)


CLEAN += initcode.c
initcode.c: init.elf ../tools/bintoarray/bintoarray
	@echo Generate initcode.c
	@$(OBJCOPY) init.elf /dev/null \
		--dump-section .text=/dev/stdout | \
		../tools/bintoarray/bintoarray initcodetext \
			> initcode.c
	@$(OBJCOPY) init.elf /dev/null \
		--dump-section .data=/dev/stdout | \
		../tools/bintoarray/bintoarray initcodedata \
			>> initcode.c


CLEAN += am335x.umg
am335x.umg: am335x.bin ../tools/mkuboot/mkuboot
	@echo MKUBOOT $@
	@../tools/mkuboot/mkuboot -a arm \
		-e ${loadaddr} -l ${loadaddr} -o linux \
		am335x.bin am335x.umg


.PHONY: clean 
clean: 
	@echo cleaning
	@rm -f $(CLEAN)


.PHONY: copy
copy: am335x.umg
	cp am335x.umg /var/ftpd/

