#!/bin/bash

set -x


if mount | grep -q "^/dev/sdb1 "; then
    echo "/dev/sdb1 is mounted"
else
    sudo mount /dev/sdb1 /mnt/sdcard/
fi
sudo cp kernel.fit /mnt/sdcard/
sudo cp boot.scr /mnt/sdcard/
sudo cp kernel.bin /mnt/sdcard/
sudo sync
sudo umount /mnt/sdcard