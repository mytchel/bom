init_src := \
	init/heap.c init/com.c \
	init/main.c init/pipe.c init/fs.c  \
	init/mount.c init/uart.c init/mmc.c \
	../lib/fs.c ../lib/misc.c

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
