#
#    Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

.SUFFIXES: .S .c .h .o .elf .bin .list .umg

CLEAN = 

all: am335x.list am335x.umg copy

loadaddr=0x82000000

DEBUG ?= 0
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

include mkuboot/local.mk
include bintoarray/local.mk
include init/local.mk

A_SRC := \
        asm.S                   \
        vectors.S

C_SRC := \
        init/initcode.c         \
        main.c                  \
        trap.c                  \
        uart.c                  \
        timer.c                 \
        mem.c                   \
        proc.c                  \
        syscall.c               \
        mmu.c

P_SRC := \
        ../port/com.c           \
        ../port/proc.c          \
        ../port/lock.c          \
        ../port/segment.c       \
        ../port/fgroup.c        \
        ../port/ngroup.c        \
        ../port/binding.c       \
        ../port/chan.c          \
        ../port/pipe.c          \
        ../port/file.c          \
        ../port/mount.c         \
        ../port/root.c          \
        ../port/path.c          \
        ../port/heap.c          \
        ../port/sysproc.c       \
        ../port/sysfile.c       \
        ../port/syscall.c       \
        ../lib/fs.c             \
        ../lib/string.c         \
	../lib/misc.c


OBJECTS := \
	$(A_SRC:%.S=%.o) \
	$(C_SRC:%.c=%.o) \
	$(P_SRC:%.c=%.o)

CLEAN += $(OBJECTS)

.c.o .S.o:
	@echo CC $<
	@$(CC) $(CFLAGS) -c -o $@ $<


.elf.bin:
	@echo OBJCOPY $@
	@$(OBJCOPY) -Obinary $< $@


.elf.list:
	@echo OBJDUMP $@
	@$(OBJDUMP) -S $< > $@


am335x.elf: linker.ld $(OBJECTS)
	@echo LD $@
	@$(LD) $(LDFLAGS) -T linker.ld -o $@ \
		$(OBJECTS) \
		$(LGCC)

am335x.umg: am335x.bin mkuboot/mkuboot
	@echo MKUBOOT $@
	@./mkuboot/mkuboot -a arm \
		-e ${loadaddr} -l ${loadaddr} -o linux \
		am335x.bin am335x.umg

clean: 
	@echo cleaning
	@rm -f $(CLEAN)


.PHONY: all clean 

.PHONY: copy
copy: am335x.umg
	cp am335x.umg /var/ftpd/
