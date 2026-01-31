# ==========================================
# Toolchain Definitions
# ==========================================
CROSS_COMPILE = riscv64-unknown-elf-
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
MKIMAGE = mkimage
DTC     = dtc
QEMU    = qemu-system-riscv64

# ==========================================
# Platform Selection
# ==========================================
PLATFORM ?= board

CFLAGS_COMMON = -mcmodel=medany -ffreestanding -nostdlib -g -Wall -Iinclude

ifeq ($(PLATFORM),qemu)
    CFLAGS  = $(CFLAGS_COMMON) -DQEMU
    LDFLAGS = -T qemu.ld
    TARGET  = kernel.elf
else
    CFLAGS  = $(CFLAGS_COMMON) -DBOARD
    LDFLAGS = -T linker.ld
    TARGET  = kernel.fit
endif

# ==========================================
# File Management [重點修改區]
# ==========================================
# 1. 定義 obj 資料夾名稱
OBJ_DIR  = obj

C_SRCS   = $(wildcard src/*.c)
ASM_SRCS = $(wildcard src/*.S)

# 2. 修改 OBJS 的路徑
# 使用 patsubst 把 src/xxx.c 換成 obj/xxx.o
OBJS     = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(C_SRCS)) \
           $(patsubst src/%.S, $(OBJ_DIR)/%.o, $(ASM_SRCS))

ITS_FILE = kernel.its
DTB_FILE = x1_orangepi-rv2.dtb

# [QEMU 設定]
QEMU_FLAGS = -M virt -smp 1 -m 256M -nographic -bios default -initrd initramfs.cpio

# ==========================================
# Build Rules
# ==========================================
.PHONY: all clean qemu board run-qemu dump-dtb

all: $(TARGET)

# ---------------------------
# 便捷指令
# ---------------------------
board: 
	@echo "Building for Orange Pi RV2..."
	@$(MAKE) PLATFORM=board all

qemu: 
	@echo "Building for QEMU..."
	@$(MAKE) PLATFORM=qemu run-qemu

run-qemu: kernel.elf
	$(QEMU) $(QEMU_FLAGS) -kernel kernel.elf

dump-dtb: kernel.elf
	@echo "Dumping QEMU DTB..."
	$(QEMU) $(QEMU_FLAGS) -kernel kernel.elf -machine virt,dumpdtb=qemu.dtb
	@echo "Converting DTB to DTS..."
	$(DTC) -I dtb -O dts -o qemu.dts qemu.dtb
	@echo "Done! Check 'qemu.dts' to see the device tree structure."

# ---------------------------
# 編譯流程 [重點修改區]
# ---------------------------

# 3. 建立 obj 資料夾的規則
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

kernel.fit: kernel.bin $(ITS_FILE) $(DTB_FILE)
	@echo "Generating FIT image..."
	$(MKIMAGE) -f $(ITS_FILE) $@

kernel.bin: kernel.elf
	@echo "Generating binary..."
	$(OBJCOPY) -O binary -S $< $@

kernel.elf: $(OBJS)
	@echo "Linking kernel.elf with $(LDFLAGS)..."
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# 4. 修改編譯規則：
#    - 目標變成 $(OBJ_DIR)/%.o
#    - 加入 | $(OBJ_DIR) 依賴，確保編譯前資料夾存在
#    - 加上 -o $@ 明確指定輸出到 obj/ 下
$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: src/%.S | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) kernel.elf kernel.bin kernel.fit qemu.dtb qemu.dts