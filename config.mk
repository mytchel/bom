TARGET = am335x

CROSS_COMPILE = arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

CFLAGS := -std=c89 -O3 \
	-Wall -Werror


HOSTCC ?= cc
HOSTLD ?= ld

HOSTCFLAGS := -std=c89 -O3 \
	-Wall -Werror


