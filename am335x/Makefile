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

LDFLAGS := -nostdlib -nodefaultlibs \
	-T linker.ld

HOST_CFLAGS := 
HOST_LDFLAGS :=

.include <mkuboot/Makefile>

HEADERS := *.h ../port/*.h

A_SRC := \
	startup.S		\
	vectors.S		\
	syscall.S

C_SRC := \
	abort.c			\
	uart.c			\
	intc.c			\
	proc.c			\
	timer.c			\
	watchdog.c		\
	mem.c

P_SRC := \
	../port/com.c		\
	../port/sched.c		\
	../port/syscall.c	\
	../port/main.c
	
OBJECTS := \
	$(A_SRC:%.S=%.o) \
	$(C_SRC:%.c=%.o) \
	$(P_SRC:%.c=%.o)

CLEAN += $(OBJECTS)

$(OBJECTS): $(HEADERS)
	@echo CC $@
	@$(CC) $(CFLAGS) -c -o $@ $<


am335x.elf: linker.ld $(OBJECTS)
	@echo LD $@
	@$(LD) $(LDFLAGS) -o $@ \
		$(OBJECTS) \
		-L/usr/local/lib/gcc/arm-none-eabi/4.9.3/ -lgcc


am335x.bin: am335x.elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -Obinary am335x.elf am335x.bin


am335x.list: am335x.elf
	@echo OBJDUMP $@
	@$(OBJDUMP) -S am335x.elf > am335x.list


am335x.umg: am335x.bin mkuboot/mkuboot
	@echo MKUBOOT $@
	@./mkuboot/mkuboot -a arm \
		-e ${loadaddr} -l ${loadaddr} -o linux \
		am335x.bin am335x.umg

clean: 
	@echo cleaning
	@rm -f $(CLEAN)


.PHONY: all clean 
