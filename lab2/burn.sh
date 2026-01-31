#!/bin/bash

set -e

sudo mount /dev/sdb1 /mnt/sdcard/
sudo cp kernel.fit /mnt/sdcard/
sudo sync
sudo umount /mnt/sdcard