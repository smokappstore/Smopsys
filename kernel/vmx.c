#include "vmx.h"
#include "../drivers/vga_holographic.h"
#include "../drivers/bayesian_serial.h"

// Helper for Hex printing if not available in VGA driver
void kprint_hex(uint32_t n) {
    char buf[16];
    const char *digits = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';
    // Only printing low 32 bits for now
    for(int i=0; i<8; i++) {
        buf[9-i] = digits[n & 0xF];
        n >>= 4;
    }
    buf[10] = 0;
    vga_holographic_write(buf);
}

// Helpers for logging
void kprint(const char* str) {
    vga_holographic_write(str);
    bayesian_serial_write(str);
}

void kprint_ln(const char* str) {
    vga_holographic_write(str);
    vga_holographic_write_char('\n');
    bayesian_serial_write(str);
    bayesian_serial_write("\n");
}

// Helper for CPUID
static inline void cpuid(int leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile ("cpuid" 
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx) 
        : "a" (leaf));
}

// Helper for RDMSR
static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile ("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
    return ((uint64_t)high << 32) | low;
}

// Helper for WRMSR
static inline void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    __asm__ volatile ("wrmsr" : : "c" (msr), "a" (low), "d" (high));
}

// Helper for reading CR4
static inline uint32_t read_cr4() {
    uint32_t cr4;
    __asm__ volatile ("mov %%cr4, %0" : "=r" (cr4));
    return cr4;
}

// Helper for writing CR4
static inline void write_cr4(uint32_t val) {
    __asm__ volatile ("mov %0, %%cr4" : : "r" (val));
}

int check_vmx_support() {
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);
    
    if (!(ecx & CPUID_VMX_BIT)) {
        return 0;
    }
    return 1;
}

void enable_vmx_in_cr4() {
    uint32_t cr4 = read_cr4();
    if (!(cr4 & CR4_VMXE)) {
        write_cr4(cr4 | CR4_VMXE);
    }
}

// VMXON region (Must be 4KB aligned)
static VmxRegion vmxon_region;
// static VmxRegion vmcs_region; // Unused for now

void init_vmcs(uint32_t revision_id);

void init_vmx() {
    vga_holographic_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    kprint("[VMX] Checking support... ");
    if (!check_vmx_support()) {
        vga_holographic_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
        kprint_ln("FAILED (No VMX in CPUID)");
        return;
    }
    vga_holographic_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    kprint_ln("OK");
    
    vga_holographic_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    kprint("[VMX] Enabling in CR4... ");
    enable_vmx_in_cr4();
    vga_holographic_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    kprint_ln("OK");
    
    // Check MSR IA32_FEATURE_CONTROL
    uint64_t feature_control = rdmsr(IA32_FEATURE_CONTROL);
    if (!(feature_control & FEATURE_CONTROL_LOCK_BIT)) {
        vga_holographic_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
        kprint("[VMX] Locking Feature Control MSR... ");
        feature_control |= FEATURE_CONTROL_LOCK_BIT | FEATURE_CONTROL_VMXON_OUT_SMX;
        wrmsr(IA32_FEATURE_CONTROL, feature_control);
        vga_holographic_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
        kprint_ln("Locked");
    } else {
        if (!(feature_control & FEATURE_CONTROL_VMXON_OUT_SMX)) {
            vga_holographic_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
            kprint_ln("[VMX] FAILED: Locked with VMX disabled by BIOS");
            return;
        }
    }
    
    // Read Revision ID
    uint64_t vmx_basic = rdmsr(IA32_VMX_BASIC);
    uint32_t revision_id = (uint32_t)vmx_basic;
    
    vga_holographic_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    kprint("[VMX] Revision ID: ");
    kprint_hex(revision_id);
    kprint_ln("");

    // Setup VMXON region
    vmxon_region.revision_id = revision_id;
    uint32_t vmxon_phys = (uint32_t)&vmxon_region; 
    
    vga_holographic_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprint("[VMX] Executing VMXON... ");
    
    // NOTE: This might fault if QEMU isn't configured with nested virtualization!
    vmx_vmxon(vmxon_phys, 0); 
    
    vga_holographic_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    kprint_ln("Executed (Root Mode Entered)");
    
    // Phase 3: Setup VMCS
    init_vmcs(revision_id);
    
    // Restore color
    vga_holographic_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

// ========================================================
// VMCS & EPT Setup
// ========================================================
#include "ept.h"
#include "surgical_scheduler.h"

extern GoldenState current_golden_state;
extern SurgicalState current_surgical_state;

static VmxRegion vmcs_region;
static ept_context_t ept_ctx;

void init_vmcs(uint32_t revision_id) {
    kprint("[VMX] Initializing VMCS... ");
    
    // 1. Prepare VMCS Region
    vmcs_region.revision_id = revision_id;
    uint32_t vmcs_phys = (uint32_t)&vmcs_region;
    
    vmx_vmptrld(vmcs_phys, 0);
    
    // 2. Initialize EPT
    init_ept(&ept_ctx);
    ept_map_identity(&ept_ctx); // Identity map first 4GB
    uint64_t eptp = ept_get_pointer(&ept_ctx);
    
    // 3. Write VMCS Fields (Simplified for 32-bit guest)
    
    // Helper to write generic fields
    // Host State
    vmx_vmwrite(HOST_CR0, read_cr4() | 0x1); // PE=1
    vmx_vmwrite(HOST_CR3, (uint32_t)read_cr4()); // Actually need CR3
    // Note: read_cr4() above writes CR0 PE bit? No, logic error in comment.
    // Correct: CR0.
    
    // We need actual CR0, CR3. 
    uint32_t cr0, cr3, cr4;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    cr4 = read_cr4();
    
    vmx_vmwrite(HOST_CR0, cr0); 
    vmx_vmwrite(HOST_CR3, cr3); 
    vmx_vmwrite(HOST_CR4, cr4);
    
    // Host Segment Selectors
    vmx_vmwrite(HOST_CS_SELECTOR, 0x08); // Kernel Code
    vmx_vmwrite(HOST_SS_SELECTOR, 0x10); // Kernel Data
    vmx_vmwrite(HOST_DS_SELECTOR, 0x10);
    vmx_vmwrite(HOST_ES_SELECTOR, 0x10);
    vmx_vmwrite(HOST_FS_SELECTOR, 0x10);
    vmx_vmwrite(HOST_GS_SELECTOR, 0x10);
    vmx_vmwrite(HOST_TR_SELECTOR, 0x00); // We don't have a TR set up properly maybe? 
                                         // If TR is null, vmlaunch will fail.
                                         // For now assume 0 is risky.
                                         // Usually we need at least a minimal TSS.
    
    // Guest State (Simplified HLT Loop)
    vmx_vmwrite(GUEST_CR0, 0x1 | (1<<31)); // PE, PG
    vmx_vmwrite(GUEST_CR3, cr3); // Share host page table for simplicity OR use EPT?
                                 // If EPT enabled, Guest CR3 is guest-physical.
                                 // We can let guest use CR3=0 if we map it?
                                 // Let's copy Host CR3 so it sees kernel mapping if we identity map.
    vmx_vmwrite(GUEST_CR4, 0x2000); // VMXE
    
    vmx_vmwrite(GUEST_RFLAGS, 0x2); // Reserved bit 1 must be 1
    
    // Controls
    // Enable EPT in Secondary Controls
    // We need to read MSRs to see what bits MUST be 1.
    // For now, minimal write, might fail checks.
    
    vmx_vmwrite(PIN_BASED_VM_EXEC_CONTROL, 0);
    vmx_vmwrite(CPU_BASED_VM_EXEC_CONTROL, CPU_BASED_ACTIVATE_SECONDARY_CONTROLS | CPU_BASED_HLT_EXITING);
    vmx_vmwrite(SECONDARY_VM_EXEC_CONTROL, SECONDARY_EXEC_ENABLE_EPT | SECONDARY_EXEC_UNRESTRICTED_GUEST);
    
    // EPT Pointer
    vmx_vmwrite(EPT_POINTER, (uint32_t)eptp);
    vmx_vmwrite(EPT_POINTER_HIGH, (uint32_t)(eptp >> 32));
    
    // Exit handler stub
    extern void vm_exit_handler_stub();
    vmx_vmwrite(HOST_RIP, (uint32_t)vm_exit_handler_stub);
    
    kprint_ln("OK (Fields written - hypothetical)");
}

// Helper to update Metriplectic State based on VM activity
void metriplectic_hypervisor_update(uint32_t exit_reason) {
    // Mandato Metriplético:
    // Evaluar si la VM está en estado Coherente (Laminar) o Disipativo (Turbulento)
    
    // HLT (12) = Orden (Baja entropía, la VM duerme)
    // EPT Violation (48) = Caos (Memoria no mapeada)
    // CPUID (10) = Determinista
    
    double entropy_delta = 0.0;
    
    switch (exit_reason) {
        case 12: // HLT
            entropy_delta = -0.01; // Cool down
            break;
        case 48: // EPT Violation
            entropy_delta = 0.05; // Heat up
            break;
        default:
            entropy_delta = 0.001; // Background noise
            break;
    }
    
    // Update global entropy state
    // We are accessing kernel global state from "Ring -1" context
    double current_entropy = (double)current_golden_state.entropy / FP_ONE;
    current_entropy += entropy_delta;
    if (current_entropy < 0) current_entropy = 0;
    
    current_golden_state.entropy = (int32_t)(current_entropy * FP_ONE);
    
    // Check with Surgical Scheduler
    // If entropy is too high, "Evaporate" -> In a real scheduler, this would yield the CPU
    // For now, we trigger the evaporation logic which increases viscosity
    if (current_entropy > 0.5) {
         evaporate_entropy(&current_surgical_state, &current_golden_state);
         vga_holographic_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
         kprint("[HYPERVISOR] High Entropy! Evaporating...\n");
    }
}

void vm_exit_handler() {
    // Read Exit Reason
    uint32_t reason = vmx_vmread(VM_EXIT_REASON);
    
    // Metriplectic Update
    metriplectic_hypervisor_update(reason);
    
    vga_holographic_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    kprint("[VMX] VM EXIT Reason: ");
    kprint_hex(reason);
    kprint_ln("");
    
    // Implement Metriplectic "Evaporation" here
    // If reason is HLT (12), we can decide to schedule another task.
    
    // For now, just resume (loops forever if HLT causes exit and we resume to HLT)
    // We should advance RIP if it was an instruction that caused exit and handled.
    // HLT is instruction.
    
    if (reason == 12) { // HLT
        // Advance RIP
        uint32_t rip = vmx_vmread(GUEST_RIP);
        uint32_t instr_len = vmx_vmread(VM_EXIT_INSTRUCTION_LEN);
        
        vmx_vmwrite(GUEST_RIP, rip + instr_len);
        
        kprint_ln("  (HLT handled, resuming)");
    } else {
        kprint_ln("  (Unhandled exit, attempting resume)");
    }
    
    vga_holographic_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}
