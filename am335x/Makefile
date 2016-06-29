.SUFFIXES: .c .S .o .h .elf .bin .list .umg

CLEAN = 

all: am335x.list am335x.umg

loadaddr=0x82000000

HOSTCC ?= cc
CROSS_COMPILE ?= arm-none-eabi-

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

CLEAN += am335x.*

CFLAGS :=-mcpu=cortex-a8 \
	-Wall -Werror -O2 \
	-nostdinc -ffreestanding \
	-I.

LDFLAGS := -nostdlib -nodefaultlibs

HOST_CFLAGS := 
HOST_LDFLAGS :=

.include <mkuboot/local.mk>
.include <bintoarray/local.mk>

HEADERS := init.h *.h ../port/*.h ../include/*.h

A_SRC := \
	asm.S			\
	vectors.S		\
	user_syscalls.S

C_SRC := \
	main.c			\
	trap.c			\
	uart.c			\
	timer.c			\
	watchdog.c		\
	mem.c			\
	syscall.c		\
	mmu.c

P_SRC := \
	../port/misc.c		\
	../port/com.c		\
	../port/proc.c		\
	../port/syscall.c	\
	../port/segment.c
	
OBJECTS := \
	$(A_SRC:%.S=%.o) \
	$(C_SRC:%.c=%.o) \
	$(P_SRC:%.c=%.o)

CLEAN += $(OBJECTS)

$(OBJECTS): $(HEADERS)
	@echo CC $<
	@$(CC) $(CFLAGS) -c -o $@ $<


.elf.bin:
	@echo OBJCOPY $@
	@$(OBJCOPY) -Obinary $< $@


.elf.list:
	@echo OBJDUMP $@
	@$(OBJDUMP) -S $< > $@


am335x.elf: linker.ld $(OBJECTS)
	@echo LD $@
	@$(LD) $(LDFLAGS) -T linker.ld -o $@ \
		$(OBJECTS) \
		-L/usr/local/lib/gcc/arm-none-eabi/4.9.3/ -lgcc


CLEAN += init.elf init.bin init.list init.h
init.elf: init.ld init.c user_syscalls.S uart.c ../include/*.h
	@echo CC $@
	@$(CC) $(CFLAGS) -o $@ init.c user_syscalls.S uart.c \
		$(LDFLAGS) -T init.ld \
		-L/usr/local/lib/gcc/arm-none-eabi/4.9.3/ -lgcc


init.h: init.bin bintoarray/bintoarray
	@echo "BINTOARRAY initcode init.bin > init.h"
	@./bintoarray/bintoarray initcode init.bin > init.h


am335x.umg: am335x.bin mkuboot/mkuboot
	@echo MKUBOOT $@
	@./mkuboot/mkuboot -a arm \
		-e ${loadaddr} -l ${loadaddr} -o linux \
		am335x.bin am335x.umg
	
clean: 
	@echo cleaning
	@rm -f $(CLEAN)


.PHONY: all clean 
