.SUFFIXES: .S .c .h .o .elf .bin .list .umg

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
	-std=c89 \
	-Dam335x \
	-I.

LDFLAGS := -nostdlib -nodefaultlibs
LGCC := -L/usr/local/lib/gcc/arm-none-eabi/4.9.3/ -lgcc

HOST_CFLAGS := -Wall -Werror -std=c89
HOST_LDFLAGS :=

include mkuboot/local.mk
include bintoarray/local.mk
include init/local.mk

A_SRC := \
	asm.S			\
	vectors.S

C_SRC := \
	init/initcode.c		\
	main.c			\
	trap.c			\
	uart.c			\
	timer.c			\
	mem.c			\
	syscall.c		\
	mmu.c

P_SRC := \
	../port/com.c		\
	../port/proc.c		\
	../port/segment.c	\
	../port/fgroup.c	\
	../port/ngroup.c	\
	../port/pipe.c		\
	../port/heap.c		\
	../port/sysproc.c	\
	../port/sysfile.c	\
	../port/syscall.c
	
OBJECTS := \
	$(A_SRC:%.S=%.o) \
	$(C_SRC:%.c=%.o) \
	$(P_SRC:%.c=%.o)

CLEAN += $(OBJECTS)

.c.o .S.o:
	@echo CC $<
	@$(CC) $(CFLAGS) -c -o $@ $<


.elf.bin:
	@echo OBJCOPY $@
	@$(OBJCOPY) -Obinary $< $@


.elf.list:
	@echo OBJDUMP $@
	@$(OBJDUMP) -sS $< > $@


am335x.elf: linker.ld $(OBJECTS)
	@echo LD $@
	@$(LD) $(LDFLAGS) -T linker.ld -o $@ \
		$(OBJECTS) \
		$(LGCC)

am335x.umg: am335x.bin mkuboot/mkuboot
	@echo MKUBOOT $@
	@./mkuboot/mkuboot -a arm \
		-e ${loadaddr} -l ${loadaddr} -o linux \
		am335x.bin am335x.umg
	
clean: 
	@echo cleaning
	@rm -f $(CLEAN)


.PHONY: all clean 
