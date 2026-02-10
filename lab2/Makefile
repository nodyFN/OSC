CROSS_COMPILE = riscv64-unknown-elf-
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
MKIMAGE = mkimage
DTC     = dtc
QEMU    = qemu-system-riscv64

# Paths
OBJ_DIR     = obj
OBJ_K_DIR   = $(OBJ_DIR)/kernel
OBJ_BL_DIR  = $(OBJ_DIR)/bootloader

# Kernel Sources
K_SRC_DIR   = kernel/src
K_SRCS      = $(wildcard $(K_SRC_DIR)/*.c) $(wildcard $(K_SRC_DIR)/*.S)
K_OBJS      = $(patsubst $(K_SRC_DIR)/%.c,$(OBJ_K_DIR)/%.o,$(filter %.c,$(K_SRCS))) \
              $(patsubst $(K_SRC_DIR)/%.S,$(OBJ_K_DIR)/%.o,$(filter %.S,$(K_SRCS)))

# Bootloader Sources
BL_SRC_DIR   = bootloader/src
BL_SRCS      = $(wildcard $(BL_SRC_DIR)/*.c) $(wildcard $(BL_SRC_DIR)/*.S)
BL_OBJS      = $(patsubst $(BL_SRC_DIR)/%.c,$(OBJ_BL_DIR)/%.o,$(filter %.c,$(BL_SRCS))) \
               $(patsubst $(BL_SRC_DIR)/%.S,$(OBJ_BL_DIR)/%.o,$(filter %.S,$(BL_SRCS)))

# Files
BOOTLOADER_ELF = bootloader.elf
BOOTLOADER_BIN = bootloader.bin
KERNEL_ELF     = kernel.elf
KERNEL_BIN     = kernel.bin
TARGET_FIT     = kernel.fit
ITS_FILE       = kernel.its
MY_DTB         = x1_orangepi-rv2.dtb
MY_RAMDISK     = initramfs.cpio
BOOT_CMD       = boot.cmd
BOOT_SCR       = boot.scr

CFLAGS = -mcmodel=medany -ffreestanding -nostdlib -g -Wall -Iinclude -march=rv64gc_zifencei -mabi=lp64d

ifeq ($(PLATFORM),qemu)
    BL_LD_SCRIPT = bootloader/qemu.ld
    K_LD_SCRIPT  = kernel/qemu.ld  
    PLATFORM_DEF = -DQEMU
    LOADER_ADDR  = -DLOADER_ADDR=0x82000000
else
    BL_LD_SCRIPT = bootloader/board.ld
    K_LD_SCRIPT  = kernel/board.ld
    PLATFORM_DEF = -DBOARD
    LOADER_ADDR  = -DLOADER_ADDR=0x42000000
endif

QEMU_FLAGS = -M virt -m 256M -nographic -bios default \
             -initrd $(MY_RAMDISK) \
             -kernel $(BOOTLOADER_ELF) \
             -device loader,file=$(KERNEL_BIN),addr=0x82000000

.PHONY: all clean dirs qemu board run-qemu build-all dump-dtb

all: dirs build-all

dirs:
	@mkdir -p $(OBJ_K_DIR) $(OBJ_BL_DIR)


qemu:
	@echo "========================================"
	@echo " Building environment for QEMU ..."
	@echo "========================================"
	@$(MAKE) PLATFORM=qemu run-qemu

board:
	@echo "========================================"
	@echo " Building environment for Orange Pi RV2 ..."
	@echo "========================================"
	@$(MAKE) PLATFORM=board $(KERNEL_BIN) $(TARGET_FIT) $(BOOT_SCR)
	@echo ""
	@echo "Files for SD Card:"
	@echo "  1. $(TARGET_FIT)  (Contains Bootloader & DTB)"
	@echo "  2. $(BOOT_SCR)    (U-Boot Script)"
	@echo ""
	@echo "Prepare to send via UART:"
	@echo "  1. $(KERNEL_BIN)"
	@echo "  (Use 'python3 send_kernel.py' after booting)"

dump-dtb: $(MY_RAMDISK) $(BOOTLOADER_ELF)
	@echo "========================================"
	@echo " Dumping QEMU Device Tree Blob ..."
	@echo "========================================"
	@echo "  QEMU    Generating qemu.dtb"
	@$(QEMU) -M virt -m 256M -nographic \
             -initrd $(MY_RAMDISK) \
             -kernel $(BOOTLOADER_ELF) \
             -machine dumpdtb=qemu.dtb
	@echo "  DTC     Converting to qemu.dts"
	@$(DTC) -I dtb -O dts -o qemu.dts qemu.dtb
	@echo "----------------------------------------"
	@echo "Done! 'qemu.dts' has been generated."

build-all: $(KERNEL_BIN) $(BOOTLOADER_BIN)

run-qemu: build-all
	@echo "Running QEMU..."
	$(QEMU) -M virt -m 256M -display none \
            -serial pty \
            -monitor stdio \
            -bios default \
            -initrd $(MY_RAMDISK) \
            -kernel $(BOOTLOADER_ELF)

$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "  OBJCOPY $@"
	@$(OBJCOPY) -O binary -S $< $@

$(KERNEL_ELF): $(K_OBJS) $(K_LD_SCRIPT)
	@echo "  LD      $@ (Script: $(K_LD_SCRIPT))"
	@$(LD) -T $(K_LD_SCRIPT) -o $@ $(K_OBJS)

$(OBJ_K_DIR)/%.o: $(K_SRC_DIR)/%.c | dirs
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -Ikernel/include $(PLATFORM_DEF) -c $< -o $@

$(OBJ_K_DIR)/%.o: $(K_SRC_DIR)/%.S | dirs
	@echo "  AS      $<"
	@$(CC) $(CFLAGS) -Ikernel/include $(PLATFORM_DEF) -c $< -o $@


$(BOOTLOADER_BIN): $(BOOTLOADER_ELF)
	@echo "  OBJCOPY $@"
	@$(OBJCOPY) -O binary -S $< $@

$(BOOTLOADER_ELF): $(BL_OBJS) $(BL_LD_SCRIPT)
	@echo "  LD      $@ (Script: $(BL_LD_SCRIPT))"
	@$(LD) -T $(BL_LD_SCRIPT) -o $@ $(BL_OBJS)

$(OBJ_BL_DIR)/%.o: $(BL_SRC_DIR)/%.c | dirs
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -Ibootloader/include $(PLATFORM_DEF) $(LOADER_ADDR) -c $< -o $@

$(OBJ_BL_DIR)/%.o: $(BL_SRC_DIR)/%.S | dirs
	@echo "  AS      $<"
	@$(CC) $(CFLAGS) -Ibootloader/include $(PLATFORM_DEF) -c $< -o $@

$(TARGET_FIT): $(BOOTLOADER_BIN) $(ITS_FILE) $(MY_DTB) $(MY_RAMDISK)
	@echo "  MKIMAGE $@"

	@cp $(BOOTLOADER_BIN) payload.bin
	
	@cp $(MY_DTB) board.dtb

	@cp $(MY_RAMDISK) ramdisk.cpio

	@$(MKIMAGE) -f $(ITS_FILE) $@ > /dev/null

	@rm payload.bin board.dtb ramdisk.cpio


$(BOOT_SCR): $(BOOT_CMD)
	@echo "  MKIMAGE $@"
	@$(MKIMAGE) -C none -A riscv -T script -d $< $@

clean:
	@echo "Cleaning..."
	@rm -rf $(OBJ_DIR) *.elf *.bin *.fit *.scr payload.bin qemu.dtb qemu.dts