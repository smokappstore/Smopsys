# ============================================================
# Smopsys Q-CORE Makefile
# 
# Sistema de build para el kernel metripléctico.
# 
# Targets:
#   all       - Construir imagen completa (smopsys.bin)
#   all-full  - Construir con bootloader de 2 stages
#   kernel    - Construir solo el kernel
#   boot      - Construir bootloaders
#   test      - Ejecutar tests en host
#   run       - Ejecutar en QEMU
#   usb       - Escribir a USB (requiere sudo)
#   clean     - Limpiar archivos generados
# ============================================================

# Herramientas
CC = i686-elf-gcc
AS = nasm
LD = i686-elf-ld
OBJCOPY = i686-elf-objcopy

# Fallback a gcc si no hay cross-compiler
ifeq ($(shell which $(CC) 2>/dev/null),)
    CC = gcc -m32
    LD = ld -m elf_i386
    OBJCOPY = objcopy
endif

# Flags para cross-compilation (bare-metal x86)
CFLAGS = -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -Wall -Wextra -m32 -O2 -fno-pie -fno-pic

LDFLAGS = -T linker.ld -nostdlib

ASFLAGS = -f elf32

# Directorios
BOOT_DIR = boot
DRIVERS_DIR = drivers
KERNEL_DIR = kernel
BUILD_DIR = build
TESTS_DIR = tests

# Archivos fuente del bootloader
STAGE1_SRC = $(BOOT_DIR)/stage1.asm
STAGE2_SRC = $(BOOT_DIR)/stage2.asm
BOOT_SRC = boot.asm  # Bootloader original simple

# Archivos fuente del kernel
KERNEL_ASM_SRC = $(KERNEL_DIR)/kernel_entry.asm

KERNEL_C_SRC = $(KERNEL_DIR)/kernel_main.c \
               $(KERNEL_DIR)/golden_operator.c \
               $(KERNEL_DIR)/lindblad.c \
               $(KERNEL_DIR)/quantum_laser.c \
               $(DRIVERS_DIR)/vga_holographic.c \
               $(DRIVERS_DIR)/bayesian_serial.c

# Archivos objeto
KERNEL_ASM_OBJ = $(BUILD_DIR)/kernel_entry.o
KERNEL_C_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(KERNEL_C_SRC)))

KERNEL_OBJS = $(KERNEL_ASM_OBJ) $(KERNEL_C_OBJ)

# Productos finales
STAGE1_BIN = $(BUILD_DIR)/stage1.bin
STAGE2_BIN = $(BUILD_DIR)/stage2.bin
BOOT_BIN = $(BUILD_DIR)/boot.bin
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_IMAGE = smopsys.bin
OS_IMAGE_FULL = smopsys_full.bin

# ============================================================
# TARGETS PRINCIPALES
# ============================================================

.PHONY: all all-full kernel boot test run run-debug usb clean dirs

all: dirs $(OS_IMAGE)
	@echo "============================================"
	@echo " Smopsys Q-CORE built successfully!"
	@echo " Run with: make run"
	@echo "============================================"

all-full: dirs $(OS_IMAGE_FULL)
	@echo "============================================"
	@echo " Smopsys Q-CORE (Full Boot) built!"
	@echo " Run with: make run-full"
	@echo "============================================"

# Imagen simple (bootloader original + kernel)
$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	@echo "[IMAGE] Creating OS image..."
	cat $(BOOT_BIN) $(KERNEL_BIN) > $@
	truncate -s 65536 $@

# Imagen completa (stage1 + stage2 + kernel)
$(OS_IMAGE_FULL): $(STAGE1_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	@echo "[IMAGE] Creating full OS image..."
	cat $(STAGE1_BIN) $(STAGE2_BIN) > $@
	# Padding para alinear kernel a sector 6 (3KB desde inicio)
	truncate -s 3072 $@
	cat $(KERNEL_BIN) >> $@
	truncate -s 65536 $@

# ============================================================
# BOOTLOADERS
# ============================================================

boot: dirs $(BOOT_BIN) $(STAGE1_BIN) $(STAGE2_BIN)

$(BOOT_BIN): $(BOOT_SRC)
	@echo "[ASM] Assembling simple bootloader..."
	$(AS) -f bin $< -o $@

$(STAGE1_BIN): $(STAGE1_SRC)
	@echo "[ASM] Assembling Stage 1..."
	$(AS) -f bin $< -o $@

$(STAGE2_BIN): $(STAGE2_SRC)
	@echo "[ASM] Assembling Stage 2..."
	$(AS) -f bin $< -o $@

# ============================================================
# KERNEL
# ============================================================

kernel: dirs $(KERNEL_BIN)

$(KERNEL_BIN): $(KERNEL_OBJS) linker.ld
	@echo "[LD] Linking kernel..."
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel.elf $@

# Assembly
$(BUILD_DIR)/kernel_entry.o: $(KERNEL_DIR)/kernel_entry.asm
	@echo "[ASM] Assembling kernel entry..."
	$(AS) $(ASFLAGS) $< -o $@

# Kernel C files
$(BUILD_DIR)/kernel_main.o: $(KERNEL_DIR)/kernel_main.c
	@echo "[CC] Compiling kernel_main.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/golden_operator.o: $(KERNEL_DIR)/golden_operator.c
	@echo "[CC] Compiling golden_operator.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lindblad.o: $(KERNEL_DIR)/lindblad.c
	@echo "[CC] Compiling lindblad.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/quantum_laser.o: $(KERNEL_DIR)/quantum_laser.c
	@echo "[CC] Compiling quantum_laser.c..."
	$(CC) $(CFLAGS) -c $< -o $@

# Drivers
$(BUILD_DIR)/vga_holographic.o: $(DRIVERS_DIR)/vga_holographic.c
	@echo "[CC] Compiling vga_holographic.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bayesian_serial.o: $(DRIVERS_DIR)/bayesian_serial.c
	@echo "[CC] Compiling bayesian_serial.c..."
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================
# TESTS (Host)
# ============================================================

test: $(TESTS_DIR)/test_golden_operator
	@echo "[TEST] Running golden operator tests..."
	./$(TESTS_DIR)/test_golden_operator

$(TESTS_DIR)/test_golden_operator: $(TESTS_DIR)/test_golden_operator.c
	@echo "[CC] Compiling test_golden_operator..."
	gcc -Wall -Wextra -g -DTEST_BUILD \
		$(TESTS_DIR)/test_golden_operator.c \
		-o $@ -lm

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

run-full: $(OS_IMAGE_FULL)
	@echo "[QEMU] Starting Smopsys Q-CORE (Full Boot)..."
	qemu-system-i386 \
		-drive format=raw,file=$(OS_IMAGE_FULL) \
		-serial stdio \
		-no-reboot \
		-d guest_errors

run-debug: $(OS_IMAGE)
	@echo "[QEMU] Starting in debug mode..."
	qemu-system-i386 \
		-drive format=raw,file=$(OS_IMAGE) \
		-serial stdio \
		-no-reboot \
		-d guest_errors,int \
		-s -S

# ============================================================
# USB
# ============================================================

usb: $(OS_IMAGE)
	@echo "============================================"
	@echo " Para escribir a USB, ejecuta:"
	@echo " sudo ./write_usb.sh /dev/sdX"
	@echo "============================================"
	@echo ""
	@echo "Dispositivos disponibles:"
	@lsblk -d -o NAME,SIZE,TYPE | grep disk

# ============================================================
# UTILIDADES
# ============================================================

dirs:
	@mkdir -p $(BUILD_DIR)

clean:
	@echo "[CLEAN] Removing build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f $(OS_IMAGE) $(OS_IMAGE_FULL)
	rm -f $(TESTS_DIR)/test_golden_operator

info: $(OS_IMAGE)
	@echo "============================================"
	@echo " Smopsys Q-CORE Image Info"
	@echo "============================================"
	@ls -la $(OS_IMAGE)
	@echo ""
	@file $(OS_IMAGE)
	@echo ""
	@echo "Components:"
	@ls -la $(BUILD_DIR)/*.bin 2>/dev/null || echo "  (not built)"

help:
	@echo "Smopsys Q-CORE Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build simple image (original bootloader)"
	@echo "  all-full  - Build full image (stage1 + stage2 bootloader)"
	@echo "  test      - Run unit tests on host"
	@echo "  run       - Run in QEMU"
	@echo "  run-full  - Run full image in QEMU"
	@echo "  usb       - Show USB writing instructions"
	@echo "  clean     - Remove build artifacts"
	@echo "  info      - Show image information"
