# ============================================================
# Smopsys Q-CORE Makefile (v0.4 - 2-Stage Bootloader)
# 
# Sistema de build con bootloader de 2 etapas
# Stage 1 (MBR) → Stage 2 → Kernel
# ============================================================

CC = i686-elf-gcc
AS = nasm
LD = i686-elf-ld
OBJCOPY = i686-elf-objcopy

ifeq ($(shell which $(CC) 2>/dev/null),)
    CC = gcc -m32
    LD = ld -m elf_i386
    OBJCOPY = objcopy
endif

CFLAGS = -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -Wall -Wextra -m32 -O2 -fno-pie -fno-pic \
         -I. -Ikernel -Idrivers

LDFLAGS = -T linker.ld -nostdlib
ASFLAGS = -f elf32

KERNEL_DIR = kernel
DRIVERS_DIR = drivers
TESTS_DIR = tests
BUILD_DIR = build
TESTS_DIR = tests

# Archivos fuente del bootloader
STAGE1_SRC = $(BOOT_DIR)/stage1.asm
STAGE2_SRC = $(BOOT_DIR)/stage2.asm
BOOT_SRC = boot.asm  # Bootloader original simple

# Archivos fuente del kernel
KERNEL_ASM_SRC = $(KERNEL_DIR)kernel/kernel_entry.asm

KERNEL_C_SRC = $(KERNEL_DIR)kernel/kernel_main.c \
               $(KERNEL_DIR)kernel/golden_operator.c \
               $(KERNEL_DIR)kernel/lindblad.c \
               $(KERNEL_DIR)kernel/quantum_laser.c \
               $(DRIVERS_DIR)drivers/vga_holographic.c \
               $(DRIVERS_DIR)drivers/bayesian_serial.c

# Archivos objeto
KERNEL_ASM_OBJ = $(BUILD_DIR)kernel/kernel_entry.o \
                 $(BUILD_DIR)kernel/kernel_main.o \
                 $(BUILD_DIR)kernel/golden_operator.o \
                 $(BUILD_DIR)kernel/lindblad.o \
                 $(BUILD_DIR)kernel/quantum_laser.o \
                 $(BUILD_DIR)drivers/vga_holographic.o \
                 $(BUILD_DIR)drivers/bayesian_serial.o

KERNEL_C_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_C_SRC))

KERNEL_OBJS = $(KERNEL_ASM_OBJ) $(KERNEL_C_OBJ)

# Productos finales
STAGE1_BIN = $(BUILD_DIR)/stage1.bin

STAGE2_SRC = stage2.asm
STAGE2_BIN = $(BUILD_DIR)/stage2.bin

# ============================================================
# KERNEL
# ============================================================

KERNEL_ENTRY_SRC = $(KERNEL_DIR)/kernel_entry.asm
KERNEL_ENTRY_OBJ = $(BUILD_DIR)/kernel_entry.o

KERNEL_C_SRCS = \
    $(KERNEL_DIR)/kernel_main.c \
    $(KERNEL_DIR)/golden_operator.c \
    $(DRIVERS_DIR)/vga_holographic.c \
    $(DRIVERS_DIR)/bayesian_serial.c

KERNEL_C_OBJS = \
    $(BUILD_DIR)/kernel_main.o \
    $(BUILD_DIR)/golden_operator.o \
    $(BUILD_DIR)/vga_holographic.o \
    $(BUILD_DIR)/bayesian_serial.o

KERNEL_OBJS = $(KERNEL_ENTRY_OBJ) $(KERNEL_C_OBJS)
KERNEL_BIN = $(BUILD_DIR)/kernel.bin

# ============================================================
# PRODUCTOS FINALES
# ============================================================

OS_IMAGE = smopsys.bin

# ============================================================
# TARGETS PRINCIPALES
# ============================================================

.PHONY: all kernel boot test run clean dirs help

all: dirs $(OS_IMAGE)
	@echo "============================================"
	@echo " Smopsys Q-CORE built successfully!"
	@echo " Image: $(OS_IMAGE) (64 KB)"
	@echo " Run with: make run"
	@echo "============================================"

# Crear imagen final (stage1 + stage2 + kernel)
$(OS_IMAGE): $(STAGE1_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	@echo "[IMAGE] Creating OS image (2-stage boot)..."
	cat $(STAGE1_BIN) $(STAGE2_BIN) > $@
	@echo "[IMAGE] Padding to sector 6 (3KB)..."
	truncate -s 3072 $@
	@echo "[IMAGE] Appending kernel..."
	cat $(KERNEL_BIN) >> $@
	@echo "[IMAGE] Finalizing to 64KB..."
	truncate -s 65536 $@
	@ls -lh $@

# ============================================================
# BOOTLOADER - STAGE 1
# ============================================================

boot: dirs $(STAGE1_BIN) $(STAGE2_BIN)

$(STAGE1_BIN): $(STAGE1_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "[ASM] Assembling Stage 1 bootloader (MBR)..."
	$(AS) -f bin $< -o $@
	@echo "      Size: $$(stat -c%s $@) bytes"

# ============================================================
# BOOTLOADER - STAGE 2
# ============================================================

$(STAGE2_BIN): $(STAGE2_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "[ASM] Assembling Stage 2 bootloader..."
	$(AS) -f bin $< -o $@
	@echo "      Size: $$(stat -c%s $@) bytes"

# ============================================================
# KERNEL
# ============================================================

kernel: dirs $(KERNEL_BIN)

$(KERNEL_BIN): $(KERNEL_OBJS) linker.ld
	@echo "[LD] Linking kernel..."
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o $(BUILD_DIR)/kernel.elf
	@echo "[COPY] Converting to binary..."
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel.elf $@
	@echo "      Size: $$(stat -c%s $@) bytes"

# ============================================================
# KERNEL ENTRY (ASM)
# ============================================================

$(BUILD_DIR)/kernel_entry.o: $(KERNEL_ENTRY_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "[ASM] Assembling kernel entry..."
	$(AS) $(ASFLAGS) $< -o $@

# ============================================================
# KERNEL C FILES
# ============================================================

$(BUILD_DIR)/kernel_main.o: $(KERNEL_DIR)/kernel_main.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling kernel_main.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/golden_operator.o: $(KERNEL_DIR)/golden_operator.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling golden_operator.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/vga_holographic.o: $(DRIVERS_DIR)/vga_holographic.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling vga_holographic.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bayesian_serial.o: $(DRIVERS_DIR)/bayesian_serial.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling bayesian_serial.c..."
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================
# TESTS (Host)
# ============================================================

test: $(TESTS_DIR)/test_golden_operator
	@echo "[TEST] Running golden operator tests..."
	./$(TESTS_DIR)/test_golden_operator

$(TESTS_DIR)/test_golden_operator: $(TESTS_DIR)/test_golden_operator.c
	@mkdir -p $(TESTS_DIR)
	@echo "[CC] Compiling test_golden_operator..."
	gcc -Wall -Wextra -g -O0 \
		-I. -Ikernel -Idrivers \
		$< -o $@ -lm

# ============================================================
# QEMU
# ============================================================

run: $(OS_IMAGE)
	@echo "[QEMU] Starting Smopsys Q-CORE..."
	qemu-system-i386 \
		-drive format=raw,file=$(OS_IMAGE) \
		-serial stdio \
		-no-reboot \
		-d guest_errors

run-debug: $(OS_IMAGE)
	@echo "[QEMU] Starting in debug mode (gdb remote)..."
	qemu-system-i386 \
		-drive format=raw,file=$(OS_IMAGE) \
		-serial stdio \
		-no-reboot \
		-d guest_errors \
		-s -S

# ============================================================
# UTILITIES
# ============================================================

dirs:
	@mkdir -p $(BUILD_DIR)

clean:
	@echo "[CLEAN] Removing build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f $(OS_IMAGE)
	rm -f $(TESTS_DIR)/test_golden_operator
	@echo "[CLEAN] Done."

info: $(OS_IMAGE)
	@echo "============================================"
	@echo " Smopsys Q-CORE Image Info"
	@echo "============================================"
	@ls -lh $(OS_IMAGE)
	@echo ""
	@file $(OS_IMAGE)
	@echo ""
	@echo "Components:"
	@echo "  Stage 1: $$(stat -c%s $(STAGE1_BIN)) bytes"
	@echo "  Stage 2: $$(stat -c%s $(STAGE2_BIN)) bytes"
	@echo "  Kernel:  $$(stat -c%s $(KERNEL_BIN)) bytes"

help:
	@echo "Smopsys Q-CORE Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all           - Build complete OS image (stage1+2+kernel)"
	@echo "  kernel        - Build kernel only"
	@echo "  boot          - Build bootloaders only"
	@echo "  test          - Run unit tests on host"
	@echo "  run           - Run image in QEMU"
	@echo "  run-debug     - Run in QEMU with GDB remote"
	@echo "  info          - Show image information"
	@echo "  clean         - Remove build artifacts"
	@echo ""
	@echo "Examples:"
	@echo "  make all      # Full build"
	@echo "  make test     # Test golden operator"
	@echo "  make run      # Boot in QEMU"
	@echo ""

.DEFAULT_GOAL := help