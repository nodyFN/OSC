#!/bin/bash

set -x

target_partition=$(lsblk -o NAME,SIZE,TYPE,MOUNTPOINT | grep "sd" | grep "disk" | awk '{print $1}')
echo target_partition: $target_partition

if mount | grep -q "^/dev/${target_partition}1 "; then
    echo "/dev/${target_partition}1 is mounted"
else
    sudo mount /dev/${target_partition}1 /mnt/sdcard/
fi
sudo cp kernel.fit /mnt/sdcard/
# sudo cp boot.scr /mnt/sdcard/
# sudo cp kernel.bin /mnt/sdcard/
sudo sync
sudo umount /mnt/sdcard