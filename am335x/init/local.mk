init_src := \
	init/syscalls.S uart.c init/com.c \
	init/misc.c init/heap.c \
	init/main.c init/pipe.c init/fs.c  

init/initcode.elf: $(init_src) init/linker.ld
	@echo CC $@
	@$(CC) $(CFLAGS) -o $@ $(init_src) $(LDFLAGS) -T init/linker.ld \
		$(LGCC)

init/initcode.c: init/initcode.elf init/initcode.list bintoarray/bintoarray
	$(OBJCOPY) init/initcode.elf /dev/null --dump-section .text=/dev/stdout | \
		./bintoarray/bintoarray initcodetext > init/initcode.c
	$(OBJCOPY) init/initcode.elf /dev/null --dump-section .data=/dev/stdout | \
		./bintoarray/bintoarray initcodedata >> init/initcode.c

CLEAN += init/initcode.*
