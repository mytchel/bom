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

.SUFFIXES:
.SUFFIXES: .c .S .h .o .a .elf .bin .list .umg

BASE ?= $(.CURDIR)/..

CLEAN += am335x.umg am335x.list init.list

include $(BASE)/config.mk

loadaddr=0x82000000

SUBDIR =
SUBDIR += $(BASE)/lib/
SUBDIR += $(BASE)/lib/libc
SUBDIR += $(BASE)/lib/libctype
SUBDIR += $(BASE)/lib/libstring
SUBDIR += $(BASE)/lib/libfssrv
SUBDIR += $(BASE)/lib/libmem
SUBDIR += $(BASE)/bin
SUBDIR += $(BASE)/tools/mkuboot
SUBDIR += $(BASE)/tools/bintoarray


.PHONY: all
all: init.list am335x.list am335x.umg


ISRC = \
	init.c                  \
	com/com.c               \
	gpio/gpio.c             \
	mmc/mmchs.c             \
	mmc/emmc.c              \
	mmc/sd.c                \
        mmc/misc.c              \
        ../init/main.c          \
        ../init/mbr.c           \
        ../init/rootfs.c        \
        ../bin/shell/interp.c   \
        ../bin/shell/token.c    \
        ../bin/shell/eval.c     \
        ../bin/fatfs/fat.c      \
        ../bin/fatfs/util.c     \
        ../bin/fatfs/mount.c    \
        ../bin/led/matrix.c


IOBJECTS := $(ISRC:%.c=%.o)
CLEAN += $(IOBJECTS)


KSRC_A := \
        vectors.S               \
        asm.S


KSRC_C := \
        main.c                  \
        trap.c                  \
        uart.c                  \
        timer.c                 \
        mem.c                   \
        proc.c                  \
        mmu.c                   \
        ../kern/com.c           \
        ../kern/proc.c          \
        ../kern/lock.c          \
        ../kern/page.c          \
        ../kern/fgroup.c        \
        ../kern/ngroup.c        \
        ../kern/message.c       \
        ../kern/chan.c          \
        ../kern/pipe.c          \
        ../kern/file.c          \
        ../kern/path.c          \
        ../kern/heap.c          \
        ../kern/sysproc.c       \
        ../kern/sysfile.c       \
        ../kern/systab.c        \
        ../kern/sysmessage.c    \
        ../kern/exec.c


KOBJECTS := \
	$(KSRC_A:%.S=%.o) \
	$(KSRC_C:%.c=%.o)


CLEAN += $(KOBJECTS)


.c.o .S.o:
	$(CC) $(CFLAGS) -c $< -o $@


.o.a:
	$(AR) rcs $@ $<


.elf.bin:
	@$(OBJCOPY) -Obinary $< $@


.elf.list:
	@$(OBJDUMP) -S $< > $@


.PHONY: $(SUBDIR)
.for sub in $(SUBDIR)
$(sub):
	@echo making $@
	$(MAKE) -C $(sub) BASE="$(BASE)"
.endfor


CLEAN += init.elf init.list
init.elf: init.ld $(IOBJECTS) $(BASE)/lib/libc $(BASE)/lib/libctype $(BASE)/lib/libfssrv $(BASE)/lib/libstring $(BASE)/lib/libmem
	@echo deps $?
	$(LD) $(LDFLAGS) \
		-T init.ld \
		-o $@ \
		$(IOBJECTS) \
		-lc -lctype -lfssrv -lstring -lmem -lgcc


CLEAN += initcode.c initcode.o
initcode.c: init.elf
	$(MAKE) -C $(BASE)/tools/bintoarray BASE="$(BASE)"
	@echo Generate initcode.c
	$(OBJCOPY) init.elf /dev/null \
		--dump-section .text=/dev/stdout | \
		$(BASE)/tools/bintoarray/bintoarray initcodetext > initcode.c
	$(OBJCOPY) init.elf /dev/null \
		--dump-section .data=/dev/stdout | \
		$(BASE)/tools/bintoarray/bintoarray initcodedata >> initcode.c


CLEAN += $(TARGET).elf $(TARGET).list
am335x.elf: kernel.ld $(KOBJECTS) initcode.o $(BASE)/lib/libctype \
	$(BASE)/lib/libfssrv $(BASE)/lib/libstring $(BASE)/lib/libmem
	$(LD) $(LDFLAGS) \
		-T kernel.ld -Ttext $(loadaddr) \
		-o $@ $(KOBJECTS) initcode.o \
		-lctype -lfssrv -lstring -lmem -lgcc


CLEAN += am335x.elf am335x.bin
am335x.umg: am335x.bin
	$(MAKE) -C $(BASE)/tools/mkuboot BASE="$(BASE)"
	$(BASE)/tools/mkuboot/mkuboot -a arm \
		-e $(loadaddr) -l $(loadaddr) -o linux \
		am335x.bin am335x.umg
	scp am335x.umg root@gibson:/var/ftpd


.PHONY: clean 
clean: 
	rm -f $(CLEAN)
.for sub in $(SUBDIR)
	$(MAKE) -C $(sub) clean \
		BASE="$(BASE)"
.endfor

