# ============================================================
# Smopsys Q-CORE Makefile
# 
# Sistema de build para el kernel metripléctico.
# 
# Targets:
#   all       - Construir imagen completa (smopsys.bin)
#   kernel    - Construir solo el kernel
#   boot      - Construir solo el bootloader
#   test      - Ejecutar tests en host
#   run       - Ejecutar en QEMU
#   clean     - Limpiar archivos generados
# ============================================================

# Herramientas
CC = i686-elf-gcc
AS = nasm
LD = i686-elf-ld
OBJCOPY = i686-elf-objcopy

# Flags para cross-compilation (bare-metal x86)
CFLAGS = -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -Wall -Wextra -m32 -O2 -fno-pie -fno-pic

LDFLAGS = -T linker.ld -nostdlib

ASFLAGS = -f elf32

# Directorios
DRIVERS_DIR = drivers
KERNEL_DIR = kernel
BUILD_DIR = build
TESTS_DIR = tests

# Archivos fuente
BOOT_SRC = boot.asm

KERNEL_ASM_SRC = $(KERNEL_DIR)/kernel_entry.asm

KERNEL_C_SRC = $(KERNEL_DIR)/kernel_main.c \
               $(KERNEL_DIR)/golden_operator.c \
               $(DRIVERS_DIR)/vga_holographic.c \
               $(DRIVERS_DIR)/bayesian_serial.c

# Archivos objeto
KERNEL_ASM_OBJ = $(BUILD_DIR)/kernel_entry.o
KERNEL_C_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(KERNEL_C_SRC)))

KERNEL_OBJS = $(KERNEL_ASM_OBJ) $(KERNEL_C_OBJ)

# Productos finales
BOOT_BIN = $(BUILD_DIR)/boot.bin
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_IMAGE = smopsys.bin

# ============================================================
# TARGETS PRINCIPALES
# ============================================================

.PHONY: all kernel boot test run clean dirs

all: dirs $(OS_IMAGE)
	@echo "============================================"
	@echo " Smopsys Q-CORE built successfully!"
	@echo " Run with: make run"
	@echo "============================================"

# Crear imagen del OS (bootloader + kernel)
$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	@echo "[IMAGE] Creating OS image..."
	cat $(BOOT_BIN) $(KERNEL_BIN) > $@
	@# Padding a 32KB mínimo para QEMU
	truncate -s 32768 $@

# ============================================================
# BOOTLOADER
# ============================================================

boot: dirs $(BOOT_BIN)

$(BOOT_BIN): $(BOOT_SRC)
	@echo "[ASM] Assembling bootloader..."
	$(AS) -f bin $(BOOT_SRC) -o $@

# ============================================================
# KERNEL
# ============================================================

kernel: dirs $(KERNEL_BIN)

$(KERNEL_BIN): $(KERNEL_OBJS) linker.ld
	@echo "[LD] Linking kernel..."
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel.elf $@

# Ensamblar kernel entry
$(BUILD_DIR)/kernel_entry.o: $(KERNEL_DIR)/kernel_entry.asm
	@echo "[ASM] Assembling kernel entry..."
	$(AS) $(ASFLAGS) $< -o $@

# Compilar archivos C del kernel
$(BUILD_DIR)/kernel_main.o: $(KERNEL_DIR)/kernel_main.c
	@echo "[CC] Compiling kernel_main.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/golden_operator.o: $(KERNEL_DIR)/golden_operator.c
	@echo "[CC] Compiling golden_operator.c..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar drivers
$(BUILD_DIR)/vga_holographic.o: $(DRIVERS_DIR)/vga_holographic.c
	@echo "[CC] Compiling vga_holographic.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bayesian_serial.o: $(DRIVERS_DIR)/bayesian_serial.c
	@echo "[CC] Compiling bayesian_serial.c..."
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================
# TESTS (Ejecutar en host, no bare-metal)
# ============================================================

test: $(TESTS_DIR)/test_golden_operator
	@echo "[TEST] Running golden operator tests..."
	./$(TESTS_DIR)/test_golden_operator

$(TESTS_DIR)/test_golden_operator: $(TESTS_DIR)/test_golden_operator.c $(KERNEL_DIR)/golden_operator.c
	@echo "[CC] Compiling test_golden_operator..."
	gcc -Wall -Wextra -g -DTEST_BUILD -lm \
		$(TESTS_DIR)/test_golden_operator.c \
		-o $@

# ============================================================
# EJECUCIÓN EN QEMU
# ============================================================

run: $(OS_IMAGE)
	@echo "[QEMU] Starting Smopsys Q-CORE..."
	qemu-system-i386 \
		-drive format=raw,file=$(OS_IMAGE) \
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
# UTILIDADES
# ============================================================

dirs:
	@mkdir -p $(BUILD_DIR)

clean:
	@echo "[CLEAN] Removing build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f $(OS_IMAGE)
	rm -f $(TESTS_DIR)/test_golden_operator

# Mostrar información de la imagen
info: $(OS_IMAGE)
	@echo "============================================"
	@echo " Smopsys Q-CORE Image Info"
	@echo "============================================"
	@ls -la $(OS_IMAGE)
	@echo ""
	@echo "Bootloader: $(BOOT_BIN)"
	@ls -la $(BOOT_BIN) 2>/dev/null || echo "  (not built)"
	@echo ""
	@echo "Kernel: $(KERNEL_BIN)"
	@ls -la $(KERNEL_BIN) 2>/dev/null || echo "  (not built)"
