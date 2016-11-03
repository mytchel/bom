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

KSRC_C += \
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
        ../lib/string.c         \
        ../lib/fssrv.c          \
	../lib/mem.c            \
        ../lib/ctype.c


KOBJECTS := \
	$(KSRC_A:%.S=%.o) \
	$(KSRC_C:%.c=%.o)


CLEAN += $(KOBJECTS)

CLEAN += $(TARGET).elf $(TARGET).list
$(TARGET).elf: kernel.ld $(KOBJECTS)
	@echo LD $@
	@$(LD) $(LDFLAGS) -T kernel.ld -o $@ \
		$(KOBJECTS) \
		$(LIBS)

