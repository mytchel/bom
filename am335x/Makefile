all: am335x.list am335x.umg

loadaddr=0x82000000

SUBDIRS = mkuboot

CROSS_COMPILE ?= arm-none-eabi-

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

CFLAGS :=-mcpu=cortex-a8 \
	-Wall -Werror -O2 \
	-nostdlib -nostdinc -nodefaultlibs \
	-ffreestanding

LDFLAGS := -T linker.ld \
	-nostdlib -nostdinc -nodefaultlibs

INCLUDE_DIRS := ../include .
INCLUDE_FILES :=
INCLUDES := 

.for dir in $(INCLUDE_DIRS)
INCLUDES += -I$(dir)
INCLUDE_FILES += $(dir)/*
.endfor

A_SRC := \
	src/startup.s		\
	src/vectors.s		\
	src/syscall.s

C_SRC := \
	src/abort.c		\
	src/com.c		\
	src/intc.c		\
	src/tasks.c		\
	src/timer.c		\
	../kern/com.c		\
	../kern/main.c		\
	../kern/syscall.c	\
	../kern/tasks.c

$(C_SRC:%.c=%.o): 
	@echo CC $@
	@$(CC) $(CFLAGS) -c -o $@ $> $(INCLUDES)


$(A_SRC:%.s=%.o):
	@echo CC $@
	@$(CC) $(CFLAGS) -c -o $@ $> $(INCLUDES)


am335x.elf: $(A_SRC:%.s=%.o) $(C_SRC:%.c=%.o)
	@echo LD $@
	@$(LD) $(LDFLAGS) -o $@ $>


am335x.bin: am335x.elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -Obinary am335x.elf am335x.bin


am335x.list: am335x.elf
	@echo OBJDUMP $@
	@$(OBJDUMP) -SD am335x.elf > am335x.list


mkuboot/mkuboot:
	@$(MAKE) -C mkuboot
	
am335x.umg: am335x.bin mkuboot/mkuboot
	@echo MKUBOOT $@
	@./mkuboot/mkuboot -a arm \
		-e ${loadaddr} -l ${loadaddr} -o linux \
		am335x.bin am335x.umg


clean: subdirs_clean
	@echo cleaning
	@rm -f out/* src/*.o ../kern/*.o \
		am335x.*

subdirs_clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

.PHONY: all clean subdirs_clean
