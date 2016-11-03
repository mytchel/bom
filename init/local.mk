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

ISRC_P := \
        ../init/main.c          \
        ../init/printf.c        \
        ../init/heap.c          \
        ../init/tmp.c           \
        ../init/mbr.c           \
        ../init/fat/fat.c       \
        ../init/fat/util.c      \
        ../init/fat/mount.c     \
        ../init/shell/shell.c   \
        ../init/shell/cmds.c    \
        ../lib/fssrv.c          \
        ../lib/mem.c            \
        ../lib/string.c         \
        ../lib/ctype.c


IOBJECTS := \
	$(ISRC_A:%.S=%.o) \
	$(ISRC_C:%.c=%.o) \
	$(ISRC_P:%.c=%.o)


CLEAN += $(IOBJECTS)


CLEAN += init.elf init.list
init.elf: init.ld $(IOBJECTS)
	@echo LD $@
	@$(LD) $(LDFLAGS) -T init.ld -o $@ \
		$(IOBJECTS) \
		$(LIBS)
