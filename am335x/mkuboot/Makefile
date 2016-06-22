mkuboot_src = mkuboot/mkuboot.c mkuboot/copy_elf32.c mkuboot/copy_elf64.c

mkuboot/mkuboot: $(mkuboot_src)
	@echo HOSTCC $@
	@$(HOSTCC) $(HOST_CFLAGS) -o $@ $(mkuboot_src) $(HOST_LDFLAGS) -lz

CLEAN += mkuboot/mkuboot
