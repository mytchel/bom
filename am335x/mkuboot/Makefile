src = mkuboot.c copy_elf32.c copy_elf64.c

mkuboot: $(src)
	@echo CC mkuboot
	@cc -o mkuboot $(src) -lz

clean:
	@rm -f mkuboot

.PHONY: clean
