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

.SUFFIXES: .S .c .h .o .elf .bin .list .umg

CLEAN = 
TARGET ?= am335x
HOSTCC ?= cc

CFLAGS := -std=c89 -O3 \
	-Wall -Werror \
        -nostdinc -ffreestanding \
        -D_$(TARGET)_ \
        -I${.CURDIR}/include 

LDFLAGS := -nostdlib -nodefaultlibs
LIBS:=

HOST_CFLAGS := -Wall -Werror -std=c89
HOST_LDFLAGS :=


TARGETS := $(TARGET).elf $(TARGET).list init.list

include $(TARGET)/local.mk


ISRC_P := \
        init/main.c         \
        init/printf.c        \
        init/heap.c          \
        init/tmp.c          \
        init/mbr.c           \
        init/fat/fat.c       \
        init/fat/util.c     \
        init/fat/mount.c     \
        init/shell/shell.c   \
        init/shell/cmds.c    \
        lib/fssrv.c          \
        lib/mem.c            \
        lib/string.c         \
        lib/ctype.c


KSRC_P := \
        kern/com.c           \
        kern/proc.c          \
        kern/lock.c          \
        kern/page.c          \
        kern/fgroup.c        \
        kern/ngroup.c        \
        kern/chan.c          \
        kern/pipe.c          \
        kern/file.c          \
        kern/mount.c         \
        kern/rootfs.c        \
        kern/path.c          \
        kern/heap.c          \
        kern/sysproc.c       \
        kern/sysfile.c       \
        kern/syscall.c       \
        lib/string.c         \
        lib/fssrv.c          \
	lib/mem.c            \
        lib/ctype.c


IOBJECTS := \
	$(ISRC_A:%.S=%.o) \
	$(ISRC_C:%.c=%.o) \
	$(ISRC_P:%.c=%.o)


KOBJECTS := \
	$(KSRC_A:%.S=%.o) \
	$(KSRC_C:%.c=%.o) \
	$(KSRC_P:%.c=%.o)


CLEAN += $(IOBJECTS) $(KOBJECTS)


include tools/mkuboot/local.mk
include tools/bintoarray/local.mk


.c.o .S.o:
	@echo CC $<
	@$(CC) $(CFLAGS) -c -o $@ $<


.elf.bin:
	@echo OBJCOPY $@
	@$(OBJCOPY) -Obinary $< $@


.elf.list:
	@echo OBJDUMP $@
	@$(OBJDUMP) -S $< > $@


CLEAN += init.elf init.list
init.elf: $(TARGET)/init.ld $(IOBJECTS)
	@echo LD $@
	@$(LD) $(LDFLAGS) -T $(TARGET)/init.ld -o $@ \
		$(IOBJECTS) \
		$(LIBS)


CLEAN += initcode.c initcode.o
initcode.c: init.elf tools/bintoarray/bintoarray
	@echo Generate initcode.c
	@$(OBJCOPY) init.elf /dev/null \
		--dump-section .text=/dev/stdout | \
		./tools/bintoarray/bintoarray initcodetext \
			> initcode.c
	@$(OBJCOPY) init.elf /dev/null \
		--dump-section .data=/dev/stdout | \
		./tools/bintoarray/bintoarray initcodedata \
			>> initcode.c


CLEAN += $(TARGET).elf $(TARGET).list
$(TARGET).elf: $(TARGET)/kernel.ld initcode.o $(KOBJECTS)
	@echo LD $@
	@$(LD) $(LDFLAGS) -T $(TARGET)/kernel.ld -o $@ \
		$(KOBJECTS) initcode.o \
		$(LIBS)


.PHONY: clean 
clean: 
	@echo cleaning
	@rm -f $(CLEAN)

