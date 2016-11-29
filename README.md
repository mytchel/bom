# Bom

## Bundle Of Mess

Bom is a micro kernel that makes extensive use of virtual file
systems.

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
Or you can download a raw image of the file system with the kernel and
utilities at some point in the recent past from
[here](https://lackname.org/dump/bom-am335x.fs).


## Usage

Once Bom boots you will be presented with something like

```
Bom Booting...
root:
message about a device...
message about a device...
message about a device...
```

Where 'message about a device...' will be one of 'sd card on mmc0' or
'emmc card on mmc1'.

From here if you enter '/dev/mmc0/a' bom will load the fat partition I
(and you, not bom, but bom will believe you assuming partition 0 is
active) assume exists on mbr partition 0 of the sd card and mount it
on '/mnt'. Bom will then bind '/mnt/bin' to '/bin' and exec '/bin/init'.

'/bin/init' is whatever program you want the start on boot. I have a
shell, also under '/bin/shell'. Surprisingly it is the shell in
'bin/shell' of this repository.

This shell is currently pretty flimsy. Horribly built too. So far it
doesn't support pattern matching but does support list joining. If
that is a term. For example I can do:

The shell has been rebuild. It is a lot better now but doesn't
support the following just yet. It does support `{command grouping;
like this}` and `conditionalls && like || this`, along with
redirection.

```

% ls /bin/mount^(fat tmp)
11    25292	26624	/bin/mountfat
11    15224	16384	/bin/mounttmp
%
% ls /dev/mmc^(0 1)
dev/mmc0:
11	2934964224	2934964224	raw
11	67108864	67108864	a
dev/mmc1:
11	3925868544	3925868544	raw
11	16777216	16777216	a
11	3909058560	3909058560	d
%

```

Which makes things a little easier.

It supports redirections such as '>', '>>', '<', and pipes. Along with
and's and or's. So this (should) work:

```

% mkdir /b
% mounttmp /b || echo mount failed. && exit
% echo hello > /b/test
% cat /b/test
hello
% echo hi there > /b/test
% cat /b/test
hi there
% echo go away >> /b/test
% cat /b/test
hi there
go away
%

```

And so on.

## Misc

Ports, feature additions and criticisms all welcome.

## Posts

- [Announcement](https://lackname.org/blog/BOM)
- [A description of IPC in Bom](https://lackname.org/blog/A%20Slightly%20More%20Technical%20Post%20About%20Bom/)
- [Creation of the shell](https://lackname.org/blog/Pratt%20Parsing/)

