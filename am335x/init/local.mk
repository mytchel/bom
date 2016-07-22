init_src := init/syscalls.S uart.c init/com.c init/fs.c init/main.c init/pipe.c

init/initcode.elf: $(init_src) init/linker.ld
	@echo CC $@
	@$(CC) $(CFLAGS) -o $@ $(init_src) $(LDFLAGS) -T init/linker.ld \
		$(LGCC)

init/initcode.c: init/initcode.bin init/initcode.list bintoarray/bintoarray
	@echo "BINTOARRAY initcode init/initcode.bin > init/initcode.c"
	@./bintoarray/bintoarray initcode init/initcode.bin > init/initcode.c

CLEAN += init/initcode.*
