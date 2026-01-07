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
CXX = i686-elf-g++

ifeq ($(shell which $(CC) 2>/dev/null),)
    CC = gcc -m32
    LD = ld -m elf_i386
    OBJCOPY = objcopy
    CXX = g++ -m32
endif

CFLAGS = -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
         -Wall -Wextra -m32 -O2 -fno-pie -fno-pic \
         -I. -Ikernel -Idrivers

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

LDFLAGS = -T linker.ld -nostdlib
ASFLAGS = -f elf32

# Directorios
KERNEL_DIR = kernel
DRIVERS_DIR = drivers
BOOT_DIR = .
BUILD_DIR = build
TESTS_DIR = tests

# Archivos fuente del bootloader
STAGE1_SRC = $(BOOT_DIR)/stage1.asm
STAGE2_SRC = $(BOOT_DIR)/stage2.asm
BOOT_SRC = $(BOOT_DIR)/boot.asm

# Archivos fuente del kernel
KERNEL_ASM_SRC = $(KERNEL_DIR)kernel/kernel_entry.asm

KERNEL_C_SRC = $(KERNEL_DIR)kernel/kernel_main.c \
               $(KERNEL_DIR)kernel/golden_operator.c \
               $(KERNEL_DIR)kernel/lindblad.c \
               $(KERNEL_DIR)kernel/quantum_laser.c \
               $(KERNEL_DIR)kernel/prn_modulator.c \
               $(DRIVERS_DIR)drivers/vga_holographic.c \
               $(DRIVERS_DIR)drivers/bayesian_serial.c

# Archivos objeto
KERNEL_ASM_OBJ = $(BUILD_DIR)kernel/kernel_entry.o \
                 $(BUILD_DIR)kernel/kernel_main.o \
                 $(BUILD_DIR)kernel/golden_operator.o \
                 $(BUILD_DIR)kernel/lindblad.o \
                 $(BUILD_DIR)kernel/quantum_laser.o \
                 $(BUILD_DIR)kernel/prn_modulator.o \
                 $(BUILD_DIR)kernel/prn_ops.o \
                 $(BUILD_DIR)drivers/vga_holographic.o \
                 $(BUILD_DIR)drivers/bayesian_serial.o \
                 $(BUILD_DIR)kernel/vmx_ops.o

KERNEL_C_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_C_SRC)) \
               $(BUILD_DIR)/kernel/vmx.o

KERNEL_OBJS = $(KERNEL_ASM_OBJ) $(KERNEL_C_OBJ)

# Productos finales
STAGE1_BIN = $(BUILD_DIR)/stage1.bin
STAGE2_BIN = $(BUILD_DIR)/stage2.bin

# ============================================================
# KERNEL
# ============================================================

KERNEL_ENTRY_SRC = $(KERNEL_DIR)/kernel_entry.asm
KERNEL_ENTRY_OBJ = $(BUILD_DIR)/kernel_entry.o

QL_SRC = ql/example.sql
QL_C = $(BUILD_DIR)/quantum_program.c

KERNEL_C_SRCS = \
    $(KERNEL_DIR)/kernel_main.c \
    $(KERNEL_DIR)/golden_operator.c \
    $(KERNEL_DIR)/lindblad.c \
    $(KERNEL_DIR)/quantum_laser.c \
    $(KERNEL_DIR)/ql_bridge.c \
    $(KERNEL_DIR)/quantum_bridge.c \
    $(KERNEL_DIR)/metriplectic_api.c \
    $(KERNEL_DIR)/prn_modulator.c \
    $(KERNEL_DIR)/dit_engine.c \
    $(QL_C) \
    $(DRIVERS_DIR)/vga_holographic.c \
    $(DRIVERS_DIR)/bayesian_serial.c \
    $(DRIVERS_DIR)/metriplectic_kbd.c \
    $(DRIVERS_DIR)/metriplectic_heartbeat.c \
    $(KERNEL_DIR)/idt.c \
    $(KERNEL_DIR)/panic.c \
    kernel/shell.c \
    $(KERNEL_DIR)/surgical_scheduler.c \
    MemoryManager.cpp

KERNEL_C_OBJS = \
    $(BUILD_DIR)/kernel_main.o \
    $(BUILD_DIR)/golden_operator.o \
    $(BUILD_DIR)/lindblad.o \
    $(BUILD_DIR)/quantum_laser.o \
    $(BUILD_DIR)/ql_bridge.o \
    $(BUILD_DIR)/metriplectic_api.o \
    $(BUILD_DIR)/prn_modulator.o \
    $(BUILD_DIR)/dit_engine.o \
    $(BUILD_DIR)/prn_ops.o \
    $(BUILD_DIR)/quantum_program.o \
    $(BUILD_DIR)/vga_holographic.o \
    $(BUILD_DIR)/bayesian_serial.o \
    $(BUILD_DIR)/metriplectic_kbd.o \
    $(BUILD_DIR)/metriplectic_heartbeat.o \
    $(BUILD_DIR)/idt.o \
    $(BUILD_DIR)/panic.o \
    $(BUILD_DIR)/interrupt_stubs.o \
    $(BUILD_DIR)/shell.o \
    $(BUILD_DIR)/MemoryManager.o \
    $(BUILD_DIR)/bimotype.o \
    $(BUILD_DIR)/surgical_scheduler.o \
    $(BUILD_DIR)/kernel/vmx.o \
    $(BUILD_DIR)/kernel/vmx_ops.o \
    $(BUILD_DIR)/kernel/ept.o \
    $(BUILD_DIR)/kernel/quantum_bridge.o


KERNEL_OBJS = $(KERNEL_ENTRY_OBJ) $(KERNEL_C_OBJS)
KERNEL_BIN = $(BUILD_DIR)/kernel.bin

# ============================================================
# PRODUCTOS FINALES
# ============================================================

OS_IMAGE = smopsys.bin

# ============================================================
# TARGETS PRINCIPALES
# ============================================================

.PHONY: all kernel boot test run clean dirs help run-iron-dome

all: dirs $(OS_IMAGE)
	@echo "============================================"
	@echo " Smopsys Q-CORE built successfully!"
	@echo " Image: $(OS_IMAGE) (64 KB)"
	@echo " Run with: make run"
	@echo "============================================"

# ============================================================
# LIMINE UTILS
# ============================================================
LIMINE_DIR = limine

# ============================================================
# DISK IMAGE (UEFI + BIOS)
# ============================================================

image: $(OS_IMAGE)

$(OS_IMAGE): $(KERNEL_BIN) $(LIMINE_DIR)/limine
	@echo "[IMAGE] Creating partitioned disk image $(OS_IMAGE)..."
	# 1. Create partition image (63MB)
	dd if=/dev/zero of=part.img bs=1M count=63
	# 2. Format as FAT32
	mkfs.vfat -F 32 -n "SMOPSYS" part.img
	# 3. Install Limine files to partition
	mmd -i part.img ::/EFI
	mmd -i part.img ::/EFI/BOOT
	mcopy -i part.img $(LIMINE_DIR)/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i part.img $(LIMINE_DIR)/BOOTIA32.EFI ::/EFI/BOOT/BOOTIA32.EFI
	mcopy -i part.img $(LIMINE_DIR)/limine-bios.sys ::/limine-bios.sys
	mcopy -i part.img limine.cfg ::/limine.cfg
	mcopy -i part.img limine.cfg ::/EFI/BOOT/limine.cfg
	mcopy -i part.img $(BUILD_DIR)/kernel.elf ::/kernel.elf
	# 4. Create disk image (64MB)
	dd if=/dev/zero of=$@ bs=1M count=64
	# 5. Create MBR partition table (Type 0xEF = EFI System Partition)
	echo "start=2048, size=129024, type=ef, bootable" | sfdisk $@
	# 6. Embed partition image into disk image at 1MB offset
	dd if=part.img of=$@ bs=1M seek=1 conv=notrunc
	# 7. Install Limine BIOS bootloader
	$(LIMINE_DIR)/limine bios-install $@
	rm part.img
	@echo "============================================"
	@echo " Image created: $(OS_IMAGE)"
	@echo "============================================"


# ============================================================
# BOOTLOADER - STAGE 1
# ============================================================

# Legacy bootloader sections removed (migrated to Limine)


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

$(BUILD_DIR)/lindblad.o: $(KERNEL_DIR)/lindblad.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling lindblad.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/quantum_laser.o: $(KERNEL_DIR)/quantum_laser.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling quantum_laser.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ql_bridge.o: $(KERNEL_DIR)/ql_bridge.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling ql_bridge.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel/quantum_bridge.o: $(KERNEL_DIR)/quantum_bridge.c
	@mkdir -p $(BUILD_DIR)/kernel
	@echo "[CC] Compiling quantum_bridge.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/metriplectic_api.o: $(KERNEL_DIR)/metriplectic_api.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling metriplectic_api.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(QL_C): $(QL_SRC) ql/smopsys_ql.py
	@mkdir -p $(BUILD_DIR)
	@echo "[QL] Compiling $< to $@..."
	python3 ql/smopsys_ql.py $< $@

$(BUILD_DIR)/quantum_program.o: $(QL_C)
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling quantum_program.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/vga_holographic.o: $(DRIVERS_DIR)/vga_holographic.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling vga_holographic.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bayesian_serial.o: $(DRIVERS_DIR)/bayesian_serial.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling bayesian_serial.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/metriplectic_kbd.o: $(DRIVERS_DIR)/metriplectic_kbd.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling metriplectic_kbd.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/shell.o: kernel/shell.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling shell.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/MemoryManager.o: MemoryManager.cpp
	@mkdir -p $(BUILD_DIR)
	@echo "[CXX] Compiling MemoryManager.cpp..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/idt.o: $(KERNEL_DIR)/idt.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling idt.c..."
	$(CC) $(CFLAGS) -c $< -o $@

build/panic.o: $(KERNEL_DIR)/panic.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling panic.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bimotype.o: kernel/bimotype.cpp
	@mkdir -p $(BUILD_DIR)
	@echo "[CXX] Compiling bimotype.cpp..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/surgical_scheduler.o: $(KERNEL_DIR)/surgical_scheduler.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling surgical_scheduler.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dit_engine.o: $(KERNEL_DIR)/dit_engine.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling dit_engine.c..."
	$(CC) $(CFLAGS) -c $< -o $@



$(BUILD_DIR)/metriplectic_heartbeat.o: $(DRIVERS_DIR)/metriplectic_heartbeat.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling metriplectic_heartbeat.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/interrupt_stubs.o: $(KERNEL_DIR)/interrupt_stubs.asm
	@mkdir -p $(BUILD_DIR)
	@echo "[ASM] Assembling interrupt stubs..."
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/prn_modulator.o: $(KERNEL_DIR)/prn_modulator.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] Compiling prn_modulator.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/prn_ops.o: $(KERNEL_DIR)/prn_ops.asm
	@mkdir -p $(BUILD_DIR)
	@echo "[ASM] Assembling prn_ops.asm..."
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/kernel/vmx_ops.o: $(KERNEL_DIR)/vmx_ops.asm
	@mkdir -p $(BUILD_DIR)/kernel
	@echo "[ASM] Assembling vmx_ops.asm..."
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/kernel/vmx.o: $(KERNEL_DIR)/vmx.c
	@mkdir -p $(BUILD_DIR)/kernel
	@echo "[CC] Compiling vmx.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel/ept.o: $(KERNEL_DIR)/ept.c
	@mkdir -p $(BUILD_DIR)/kernel
	@echo "[CC] Compiling ept.c..."
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================
# TESTS (Host)
# ============================================================

test: $(TESTS_DIR)/test_golden_operator $(TESTS_DIR)/test_memory $(TESTS_DIR)/test_surgical_scheduler $(TESTS_DIR)/test_dit_engine
	@echo "[TEST] Running golden operator tests..."
	./$(TESTS_DIR)/test_golden_operator
	@echo "[TEST] Running memory manager tests..."
	./$(TESTS_DIR)/test_memory
	@echo "[TEST] Running surgical scheduler tests..."
	./$(TESTS_DIR)/test_surgical_scheduler
	@echo "[TEST] Running DIT engine tests..."
	./$(TESTS_DIR)/test_dit_engine


$(TESTS_DIR)/test_golden_operator: $(TESTS_DIR)/test_golden_operator.c
	@mkdir -p $(TESTS_DIR)
	@echo "[CC] Compiling test_golden_operator..."
	gcc -Wall -Wextra -g -O0 \
		-I. -Ikernel -Idrivers \
		$< -o $@ -lm

$(TESTS_DIR)/test_memory: $(TESTS_DIR)/test_memory.cpp MemoryManager.cpp
	@mkdir -p $(TESTS_DIR)
	@echo "[CXX] Compiling test_memory..."
	g++ -Wall -Wextra -g -O0 \
		-I. -Ikernel -Idrivers \
		$^ -o $@ -lm

$(TESTS_DIR)/test_surgical_scheduler: tests/test_surgical_scheduler.c kernel/surgical_scheduler.c kernel/dit_engine.c
	@mkdir -p $(TESTS_DIR)
	@echo "[CC] Compiling test_surgical_scheduler..."
	gcc -Wall -Wextra -g -O0 \
		-I. -Ikernel -Idrivers \
		$^ -o $@ -lm


$(TESTS_DIR)/test_dit_engine: tests/test_dit_engine.c kernel/dit_engine.c
	@mkdir -p $(TESTS_DIR)
	@echo "[CC] Compiling test_dit_engine..."
	gcc -Wall -Wextra -g -O0 \
		-I. -Ikernel -Idrivers \
		$^ -o $@ -lm


# ============================================================
# QEMU
# ============================================================

run: $(OS_IMAGE)
	@echo "[QEMU] Starting Smopsys Q-CORE (BIOS Mode)..."
	qemu-system-i386 \
		-m 256 \
		-drive format=raw,file=$(OS_IMAGE) \
		-serial stdio \
		-no-reboot \
		-d cpu_reset,int,guest_errors \
		-D qemu.log

run-direct: $(KERNEL_BIN)
	@echo "[QEMU] Starting Smopsys Q-CORE (Direct Kernel)..."
	qemu-system-i386 \
		-m 256 \
		-kernel build/kernel.elf \
		-serial stdio \
		-no-reboot \
		-d guest_errors

run-uefi: $(OS_IMAGE)
	@echo "[QEMU] Starting Smopsys Q-CORE (UEFI Mode)..."
	qemu-system-x86_64 \
		-m 256 \
		-bios /usr/share/ovmf/OVMF.fd \
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

run-iron-dome:
	@echo "[PYTHON] Starting Iron Dome IoT Security System..."
	PYTHONPATH=. python3 -m iron_dome.main

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
	rm -f $(TESTS_DIR)/test_memory

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
	@echo "  run-iron-dome - Run Iron Dome IoT Security System (Python)"
	@echo ""
	@echo "Examples:"
	@echo "  make all      # Full build"
	@echo "  make test     # Test golden operator"
	@echo "  make run      # Boot in QEMU"
	@echo ""

.DEFAULT_GOAL := help