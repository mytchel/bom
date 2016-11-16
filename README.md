# Bom

## Bundle Of Mess

Bom is a micro kernel that makes extensive use of virtual file
systems.

[Anouncment](https://lackname.org/blog/BOM)

[A description of IPC in Bom](https://lackname.org/blog/A%20Slightly%20More%20Technical%20Post%20About%20Bom/)

Currently the beagle bone black is the only device Bom runs on. It
has support for UART, sd cards, mmc cards, timers, interrupts, etc.
Currently no networking, USB, or HDMI.


## Build

To build for the beagle bone black run

```

make config_am335x
make

```

This will compile the kernel, libraries and utilities. You will then
need to set up an sd card and copy them to it with utilities under
/bin, and the kernel somewhere. You will probably want to copy
u-boot's MLO and u-boot.img files along with a uenv.txt file.
Or you can download a raw image of the fs with the kernel and
utilities at some point in the rescent past from
[here](https://lackname.org/dump/bom-am335x.fs).


## Misc

Ports, feature additions and criticisms all welcome.
