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

init_src := \
	init/main.c init/heap.c \
	init/pipe.c init/fs.c  \
	init/com.c init/tmp.c \
	init/mmc/mmc.c init/mmc/misc.c \
	../lib/fs.c ../lib/misc.c ../lib/string.c

init_objs := init/syscalls.o $(init_src:%.c=%.o)

CLEAN += $(init_objs)

init/initcode.elf: $(init_objs) init/linker.ld
	@echo CC $@
	@$(CC) $(CFLAGS) -o $@ $(init_objs) $(LDFLAGS) -T init/linker.ld \
		$(LGCC)

init/initcode.c: init/initcode.elf init/initcode.list bintoarray/bintoarray
	@echo Generate init/initcode.c
	@$(OBJCOPY) init/initcode.elf /dev/null --dump-section .text=/dev/stdout | \
		./bintoarray/bintoarray initcodetext > init/initcode.c
	@$(OBJCOPY) init/initcode.elf /dev/null --dump-section .data=/dev/stdout | \
		./bintoarray/bintoarray initcodedata >> init/initcode.c

CLEAN += init/initcode.*
