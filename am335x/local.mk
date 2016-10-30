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

.PHONY: all
all: $(TARGETS) am335x.umg

CROSS_COMPILE ?= arm-none-eabi-

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

CFLAGS += -mcpu=cortex-a8

LDFLAGS += -L/usr/local/lib/gcc/arm-none-eabi/4.9.3/
LIBS += -lgcc

ISRC_A := \
	am335x/init/syscalls.S		\


ISRC_C := \
	am335x/init/com.c              \
	am335x/init/mmc/mmchs.c        \
	am335x/init/mmc/emmc.c         \
	am335x/init/mmc/sd.c           \
        am335x/init/mmc/misc.c


KSRC_A := \
        am335x/asm.S                   \
        am335x/vectors.S


KSRC_C := \
        am335x/main.c                  \
        am335x/trap.c                  \
        am335x/uart.c                  \
        am335x/timer.c                 \
        am335x/mem.c                   \
        am335x/proc.c                  \
        am335x/syscall.c               \
        am335x/mmu.c


loadaddr=0x82000000

CLEAN += am335x.umg am335x.bin
am335x.umg: am335x.bin tools/mkuboot/mkuboot
	@echo MKUBOOT $@
	@./tools/mkuboot/mkuboot -a arm \
		-e ${loadaddr} -l ${loadaddr} -o linux \
		am335x.bin am335x.umg
	cp am335x.umg /var/ftpd


.PHONY: clean 
clean: 
	@echo cleaning
	@rm -f $(CLEAN)

