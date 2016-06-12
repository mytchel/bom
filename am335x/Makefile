all: am335x.list am335x.umg

loadaddr=0x82000000

CROSS_COMPILE ?= arm-none-eabi-

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump


CFLAGS :=-mcpu=cortex-a8 \
	-Wall -Werror -O2 \
	-nostdlib -nostdinc -nodefaultlibs \
	-ffreestanding \
	-I.

ASFLAGS :=-mcpu=cortex-a8 \
	-nostdlib -nostdinc -nodefaultlibs \
	-ffreestanding \
	-I.


LDFLAGS := -T linker.ld \
	-nostdlib -nostdinc -nodefaultlibs

A_SRC := \
	startup.S		\
	vectors.S		\
	syscall.S

C_SRC := \
	abort.c			\
	com.c			\
	intc.c			\
	mem.c			\
	proc.c			\
	timer.c			\
	../port/com.c		\
	../port/sched.c		\
	../port/main.c

$(C_SRC:%.c=%.o):
	@echo CC $>
	@$(CC) $(CFLAGS) -c -o $@ $>


$(A_SRC:%.S=%.o):
	@echo CC $>
	@$(CC) $(ASFLAGS) -c -o $@ $>


am335x.elf: $(A_SRC:%.S=%.o) $(C_SRC:%.c=%.o) 
	@echo LD $@
	@$(LD) $(LDFLAGS) -o $@ $>


am335x.bin: am335x.elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -Obinary am335x.elf am335x.bin


am335x.list: am335x.elf
	@echo OBJDUMP $@
	@$(OBJDUMP) -S am335x.elf > am335x.list


mkuboot/mkuboot:
	@$(MAKE) -C mkuboot

	
mkuboot_clean:
	@$(MAKE) -C mkuboot clean


am335x.umg: am335x.bin mkuboot/mkuboot
	@echo MKUBOOT $@
	@./mkuboot/mkuboot -a arm \
		-e ${loadaddr} -l ${loadaddr} -o linux \
		am335x.bin am335x.umg


clean: mkuboot_clean
	@echo cleaning
	@rm -f *.o ../port/*.o \
		am335x.*


.PHONY: all clean mkuboot_clean
