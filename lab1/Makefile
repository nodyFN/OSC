# ==========================================
# Toolchain Definitions
# ==========================================
CROSS_COMPILE = riscv64-unknown-elf-
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
MKIMAGE = mkimage
QEMU    = qemu-system-riscv64

# ==========================================
# Platform Selection (Magic Happens Here)
# ==========================================
# 預設目標 (如果只打 make，預設跑 board)
PLATFORM ?= board

# 共用參數
CFLAGS_COMMON = -mcmodel=medany -ffreestanding -nostdlib -g -Wall -Iinclude

# 根據 PLATFORM 設定差異
ifeq ($(PLATFORM),qemu)
    # QEMU 模式：定義 QEMU 巨集，使用 qemu.ld
    CFLAGS  = $(CFLAGS_COMMON) -DQEMU
    LDFLAGS = -T qemu.ld
    TARGET  = kernel.elf # QEMU 吃 ELF 就好
else
    # Board 模式：使用 linker.ld
    CFLAGS  = $(CFLAGS_COMMON) -DBOARD
    LDFLAGS = -T linker.ld
    TARGET  = kernel.fit # 板子要 FIT Image
endif

# ==========================================
# File Management
# ==========================================
C_SRCS   = $(wildcard src/*.c)
ASM_SRCS = $(wildcard src/*.S)
OBJS     = $(C_SRCS:.c=.o) $(ASM_SRCS:.S=.o)

ITS_FILE = kernel.its
DTB_FILE = x1_orangepi-rv2.dtb

# ==========================================
# Build Rules
# ==========================================

# 偽目標，確保不會跟檔名衝突
.PHONY: all clean qemu board run-qemu

# 預設動作
all: $(TARGET)

# ---------------------------
# 便捷指令
# ---------------------------

# 指令: make board
# 說明: 編譯給開發板用的 kernel.fit
board: 
	@echo "Building for Orange Pi RV2..."
	@$(MAKE) PLATFORM=board all

# 指令: make qemu
# 說明: 編譯並自動執行 QEMU
qemu: 
	@echo "Building for QEMU..."
	@$(MAKE) PLATFORM=qemu run-qemu

# 實際執行 QEMU 的指令
run-qemu: kernel.elf
	$(QEMU) -M virt -smp 1 -m 256M -nographic \
		-bios default \
		-kernel kernel.elf

# ---------------------------
# 編譯流程
# ---------------------------

# Step 4: Board Only - Make FIT Image
kernel.fit: kernel.bin $(ITS_FILE) $(DTB_FILE)
	@echo "Generating FIT image..."
	$(MKIMAGE) -f $(ITS_FILE) $@

# Step 3: Generic - Binary
kernel.bin: kernel.elf
	@echo "Generating binary..."
	$(OBJCOPY) -O binary -S $< $@

# Step 2: Linking (LDFLAGS 會根據平台變動)
kernel.elf: $(OBJS)
	@echo "Linking kernel.elf with $(LDFLAGS)..."
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# Step 1: Compiling (CFLAGS 會根據平台變動)
src/%.o: src/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

src/%.o: src/%.S
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# 清除 (切換平台前強烈建議先 clean)
clean:
	rm -f src/*.o kernel.elf kernel.bin kernel.fit