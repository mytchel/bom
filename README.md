# Bom

## Bundle Of Mess

### A Minimal Operating System for the BeagleBone Black

So far it doesn't do much.

It currently has one program that (called init, not that it initialized anything)
that is compiled then converted into an array of bytes, which is then compiled and
linked into the kernel. The kernel copies this into the pages for the first process
and it get run as in user mode with its on virtual address space.

All this program does is print a little (it has direct access to the io address space,
for now), fork a few times and the forked processes do something similar.

Ports, feature additions and criticisms all welcome.

## Build

Just run make.

This should give you the file am335x/am335x.umg which you can copy onto an sdcard and
load on your beaglebone black.

U-Boot is a bit (very) finicky with the partitioning of your sdcard. You can look that
up if you really want to but the easiest way is to just flash a beaglebone black
linux or openbsd distro onto the sdcard the copy am335x/am335x.umg and am335x/uenv.txt
onto the U-Boot fat partition. Then when U-Boot loads (if you have it in sdcard mode)
it should run Bom.

I should probably figure out how to load the kernel over uart. Because the sdcard slot
is falling apart.

### Working

- Virtual memory
- Multiple processes (preemptive)
- Working system calls so far
	- fork
	- sleep
	- exit
	- pipe
	- read
	- write
	- close

### Very Helpful Information

- 9front omap port (maybe too helpful)
- [Arm Reference](http://www.google.co.uk/url?sa=t&source=web&cd=1&ved=0CCAQFjAA&url=http%3A%2F%2Fwww.altera.com%2Fliterature%2Fthird-party%2Farchives%2Fddi0100e_arm_arm.pdf&ei=B1cwTtfHJMWmhAfh8qlI&usg=AFQjCNFqDeTfS2VR6oU93FbwBoE--ggJrA)
- Cortex A Series Programmer Guide
- Arm System Developers Guide
- am335x guide
- OpenBSD armv7 kernel.
