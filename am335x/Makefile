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
#  included in all copies or substantial kernions of the Software.
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

.SUFFIXES: .umg

CLEAN += am335x.umg am335x.list init.list

.PHONY: all
all: am335x.umg am335x.list init.list

include ../common.mk
include ../config.mk

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

CFLAGS += -mcpu=cortex-a8 \
	-nostdinc -ffreestanding \
        -D_$(TARGET)_ \
        -I../include 


LDFLAGS += -nostdlib -nodefaultlibs \
	-L/usr/local/lib/gcc/arm-none-eabi/4.9.3/

LIBS += -lgcc

ISRC_A := \
	init/syscalls.S		\


ISRC_C := \
	init/com.c              \
	init/mmc/mmchs.c        \
	init/mmc/emmc.c         \
	init/mmc/sd.c           \
        init/mmc/misc.c


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


include ../kern/local.mk
include ../init/local.mk


SUBCLEAN += ../tools/mkuboot
MKUBOOT := ../tools/mkuboot/mkuboot
$(MKUBOOT):
	@$(MAKE) -C ../tools/mkuboot


SUBCLEAN += ../tools/bintoarray
BINTOARRAY := ../tools/bintoarray/bintoarray
$(BINTOARRAY):
	@$(MAKE) -C ../tools/bintoarray


CLEAN += initcode.c initcode.o
initcode.c: init.elf $(BINTOARRAY)
	@echo Generate initcode.c
	@$(OBJCOPY) init.elf /dev/null \
		--dump-section .text=/dev/stdout | \
		$(BINTOARRAY) initcodetext > initcode.c
	@$(OBJCOPY) init.elf /dev/null \
		--dump-section .data=/dev/stdout | \
		$(BINTOARRAY) initcodedata >> initcode.c


loadaddr=0x82000000

CLEAN += am335x.elf am335x.bin
am335x.umg: am335x.bin $(MKUBOOT)
	@echo MKUBOOT $@
	@../tools/mkuboot/mkuboot -a arm \
		-e ${loadaddr} -l ${loadaddr} -o linux \
		am335x.bin am335x.umg
	cp am335x.umg /var/ftpd


