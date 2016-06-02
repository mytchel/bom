.PHONY: all
all: am335x.list am335x.umg

loadaddr=0x82000000

SUBDIRS = mkuboot

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS): 
	@$(MAKE) -C $@

CROSS_COMPILE ?= arm-none-eabi-

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

CFLAGS :=-mcpu=cortex-a8 \
	-Wall -Werror -O2 \
	-nostdlib -nostdinc -nodefaultlibs \
	-ffreestanding \
	-I../include -I.

LDFLAGS := -T linker.ld \
	-nostdlib -nostdinc -nodefaultlibs

SRC := 	\
	src/startup.s		\
	src/vectors.s		\
	src/syscall.s		\
	src/abort.c		\
	src/com.c		\
	src/intc.c		\
	src/tasks.c		\
	src/timer.c		\
	../kern/com.c		\
	../kern/main.c		\
	../kern/syscall.c	\
	../kern/tasks.c


$(SRC:%.c=%.o): ../include/*.h machine/*.h
	@echo CC $@
	@$(CC) $(CFLAGS) -c -o $@ $(@:%.o=%.c) $(INCLUDES)


$(SRC:%.s=%.o):
	@echo CC $@
	@$(CC) $(CFLAGS) -c -o $@ $(@:%.o=%.s)


am335x.elf: $(SRC:%.s=%.o:%.c=%.o) machine/*.h ../include/*.h machine/*.h
	@echo LD $@
	@$(LD) $(LDFLAGS) -o $@ \
		$(SRC:%.s=%.o:%.c=%.o)


am335x.bin: am335x.elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -Obinary am335x.elf am335x.bin


am335x.list: am335x.elf
	@echo OBJDUMP $@
	@$(OBJDUMP) -SD am335x.elf > am335x.list


am335x.umg: am335x.bin
	@echo MKUBOOT $@
	@./mkuboot/mkuboot -a arm \
		-e ${loadaddr} -l ${loadaddr} -o linux \
		am335x.bin am335x.umg


.PHONY: clean
clean:
	@echo cleaning
	@rm -f out/* src/*.o ../kern/*.o \
		am335x.*
