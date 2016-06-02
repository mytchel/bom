.PHONY: all
all: out/am335x.umg out/am335x.list out/uenv.txt

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

out/am335x.elf: $(SRC) machine/*.h ../include/*.h machine/*.h
	$(CC) $(CFLAGS) -o out/am335x.elf \
		$(SRC) \
		$(LDFLAGS)

out/am335x.bin: out/am335x.elf
	$(OBJCOPY) -Obinary out/am335x.elf out/am335x.bin

out/am335x.list: out/am335x.elf
	$(OBJDUMP) -SD out/am335x.elf > out/am335x.list

loadaddr=0x82000000
out/am335x.umg: out/am335x.bin
	./mkuboot/mkuboot -a arm \
		-e ${loadaddr} -l ${loadaddr} -o linux \
		out/am335x.bin out/am335x.umg

out/uenv.txt: uenv.txt
	cp uenv.txt out/uenv.txt

.PHONY: clean
clean:
	rm -f out/*
