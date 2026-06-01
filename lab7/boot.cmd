# not use anymore

echo "========================================"
echo " Starting Custom Bootloader Environment "
echo "========================================"

echo ">> [U-Boot] Loading Bootloader FIT Image to 0x44000000..."
load mmc 0:1 0x44000000 kernel.fit

echo ">> [U-Boot] Booting Bootloader..."
bootm 0x44000000