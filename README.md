# Bom

## Bundle Of Mess

### A Minimal Operating System for the BeagleBone Black

So far it doesn't do much.

It is a micro kernel that makes extensive use of virtual file systems
for IPC.

It currently has one program that that is compiled then converted
into an array of bytes, which is then compiled and linked into the
kernel. The kernel copies this into the pages for the first process
and runs it in user mode with its on virtual address space.

So far this program spawns makes a /dev directory, spawns a file
system that mounts itself on /dev/com and writes/reads UART0 when
it is written to/read from. Then it spawns a temporary file system
that stores files in ram and mounts it on /tmp.

Next is spawns two processes for managing the mmc host controllers
that (if they find cards) will mount themselves as directory's on
/dev/mmcy (where y = 0, 1) with files raw (for the entire device),
and a, b, c, and d for each of the mbr partitions if they exist.
These files can only be read or written in 512 byte blocks.

Then a shell is spawns that currently doesn't let you do much other
than see what files exist.

## Build

Just run make.

This should give you the file am335x/am335x.umg which you can copy
onto an sdcard and load on your beaglebone black.

U-Boot is a bit (very) finicky with the partitioning of your sdcard.
You can look that up if you really want to but the easiest way is to
just flash a beaglebone black linux or openbsd distro onto the sdcard
the copy am335x/am335x.umg onto the U-Boot fat partition. Then when
U-Boot loads (if you have it in sdcard mode) it should run Bom.

## Misc

Ports, feature additions and criticisms all welcome.
