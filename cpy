#!/bin/sh

mount /dev/sd2i /mnt || exit
cp out/out.umg /mnt/
umount /mnt
