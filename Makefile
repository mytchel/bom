CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

CFLAGS =-Wall -Werror -O2 \
	-nostdlib -nodefaultlibs -ffreestanding \
	-Iinclude

LDFLAGS = -T linker.ld 

all: out/out.umg out/out.list

SRC=src/*.s src/*.c

out/out.elf: ${SRC}
	$(CC) ${CFLAGS} -o out/out.elf ${SRC} $(LDFLAGS) 
	
out/out.bin: out/out.elf
	$(CROSS_COMPILE)objcopy -Obinary out/out.elf out/out.bin

out/out.list: out/out.elf
	$(CROSS_COMPILE)objdump -S out/out.elf > out/out.list


loadaddr=0x82000000

out/out.umg: out/out.bin mkuboot/mkuboot
	./mkuboot/mkuboot -a arm -e ${loadaddr} -l ${loadaddr} -o linux out/out.bin out/out.umg

clean:
	rm -f out/*
