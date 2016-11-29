TARGET = am335x

CROSS = arm-none-eabi-

CC = $(CROSS)gcc
LD = $(CROSS)ld
AR := $(CROSS)ar
OBJCOPY = $(CROSS)objcopy
OBJDUMP = $(CROSS)objdump

CFLAGS := -std=c89 -O3 \
	-Wall -Werror \
	-mcpu=cortex-a8 \
	-nostdinc -ffreestanding \
        -D_$(TARGET)_ \
        -I$(BASE)/include 

LDFLAGS += -nostdlib -nodefaultlibs -static \
	-L/usr/local/lib/gcc/arm-none-eabi/4.9.4/ \
	-L$(BASE)/lib

LDSCRIPT := -T $(BASE)/$(TARGET)/c.ld

# Compiler chain for build tools

HOSTCC ?= cc
HOSTLD ?= ld

HOSTCFLAGS := -std=c89 -O3 \
	-Wall -Werror


