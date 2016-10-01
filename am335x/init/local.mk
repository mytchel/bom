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

init_src := \
	init/main.c init/heap.c \
	init/pipe.c init/fs.c  \
	init/com.c init/tmp.c \
	init/mmc/mmc.c init/mmc/mmcmount.c init/mmc/misc.c \
	init/mbr/mbr.c \
	../lib/fs.c ../lib/misc.c ../lib/string.c

init_objs := init/syscalls.o $(init_src:%.c=%.o)

CLEAN += $(init_objs)

init/init.elf: $(init_objs) init/linker.ld
	@echo CC $@
	@$(CC) $(CFLAGS) -o $@ $(init_objs) $(LDFLAGS) -T init/linker.ld \
		$(LGCC)

init/initcode.c: init/init.elf init/init.list bintoarray/bintoarray
	@echo Generate init/initcode.c
	@$(OBJCOPY) init/init.elf /dev/null \
		--dump-section .text=/dev/stdout | \
		./bintoarray/bintoarray initcodetext > init/initcode.c
	@$(OBJCOPY) init/init.elf /dev/null \
		--dump-section .data=/dev/stdout | \
		./bintoarray/bintoarray initcodedata >> init/initcode.c

CLEAN += init/init.* init/initcode.*
