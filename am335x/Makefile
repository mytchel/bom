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

BASE = $(.CURDIR)/..

CLEAN += am335x.umg am335x.list init.list

include $(BASE)/config.mk

loadaddr=0x82000000

SUBDIR =
SUBDIR += $(BASE)/lib
SUBDIR += $(BASE)/tools/mkuboot
SUBDIR += $(BASE)/tools/bintoarray
SUBDIR += $(BASE)/fatfs
SUBDIR += $(BASE)/tmpfs


.PHONY: all
all: \
	$(BASE)/lib $(BASE)/lib/libc.a \
	am335x.umg am335x.list init.list \
	$(BASE)/fatfs $(BASE)/tmpfs


ISRC = \
	com/com.c               \
	mmc/mmchs.c             \
	mmc/emmc.c              \
	mmc/sd.c                \
        mmc/misc.c              \
        ../init/main.c          \
        ../init/mbr.c           \
        ../fatfs/fat.c          \
        ../fatfs/util.c         \
        ../fatfs/mount.c


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
        syscall.c               \
        mmu.c                   \
        ../kern/com.c           \
        ../kern/proc.c          \
        ../kern/lock.c          \
        ../kern/page.c          \
        ../kern/fgroup.c        \
        ../kern/ngroup.c        \
        ../kern/chan.c          \
        ../kern/pipe.c          \
        ../kern/file.c          \
        ../kern/mount.c         \
        ../kern/rootfs.c        \
        ../kern/path.c          \
        ../kern/heap.c          \
        ../kern/sysproc.c       \
        ../kern/sysfile.c       \
        ../kern/systab.c        \
        ../kern/exec.c          \
        ../lib/string/string.c  \
        ../lib/fssrv/fssrv.c    \
	../lib/mem/mem.c        \
        ../lib/ctype/ctype.c


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


CLEAN += c.a c.o
c.a: c.o

CLEAN += $(BASE)/lib/libc.a c.a
$(BASE)/lib/libc.a: c.a
	cp c.a $(BASE)/lib/libc.a	


CLEAN += init.elf init.list
init.elf: init.ld $(BASE)/lib/libc.a $(BASE)/lib $(IOBJECTS)
	$(LD) -nostdlib -nodefaultlibs -static \
		-L/usr/local/lib/gcc/arm-none-eabi/4.9.3/ \
		-L$(BASE)/lib \
		-T init.ld -o $@  \
		$(IOBJECTS) \
		-lc -lgcc -lstd \
		-lctype -lfssrv -lstring -lmem


CLEAN += initcode.c initcode.o
initcode.c: init.elf $(BASE)/tools/bintoarray
	@echo Generate initcode.c
	$(OBJCOPY) init.elf /dev/null \
		--dump-section .text=/dev/stdout | \
		$(BASE)/tools/bintoarray/bintoarray initcodetext > initcode.c
	$(OBJCOPY) init.elf /dev/null \
		--dump-section .data=/dev/stdout | \
		$(BASE)/tools/bintoarray/bintoarray initcodedata >> initcode.c


CLEAN += $(TARGET).elf $(TARGET).list
am335x.elf: kernel.ld $(KOBJECTS) initcode.o
	$(LD) -nostdlib -nodefaultlibs -static \
		-L/usr/local/lib/gcc/arm-none-eabi/4.9.3/ \
		-T kernel.ld -Ttext $(loadaddr) \
		-o $@ $(KOBJECTS) initcode.o \
		-lgcc


CLEAN += am335x.elf am335x.bin
am335x.umg: am335x.bin $(BASE)/tools/mkuboot
	$(BASE)/tools/mkuboot/mkuboot -a arm \
		-e $(loadaddr) -l $(loadaddr) -o linux \
		am335x.bin am335x.umg
	cp am335x.umg /var/ftpd


.PHONY: $(SUBDIR)
.for sub in $(SUBDIR)
$(sub):
	$(MAKE) -C $(sub) BASE="$(BASE)"
.endfor


.PHONY: clean 
clean: 
	@echo cleaning
	rm -f $(CLEAN)
.for sub in $(SUBDIR)
	$(MAKE) -C $(sub) clean \
		BASE="$(BASE)"
.endfor

