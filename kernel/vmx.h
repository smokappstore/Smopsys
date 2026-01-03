#ifndef VMX_H
#define VMX_H

#include <stdint.h>

// MSRs
#define IA32_FEATURE_CONTROL    0x3A
#define IA32_VMX_BASIC          0x480
#define IA32_VMX_PINBASED_CTLS  0x481
#define IA32_VMX_PROCBASED_CTLS 0x482
#define IA32_VMX_EXIT_CTLS      0x483
#define IA32_VMX_ENTRY_CTLS     0x484
#define IA32_VMX_MISC           0x485

// IA32_FEATURE_CONTROL bits
#define FEATURE_CONTROL_LOCK_BIT        (1 << 0)
#define FEATURE_CONTROL_VMXON_OUT_SMX   (1 << 2)

// CR4 bits
#define CR4_VMXE (1 << 13)

// CPUID
#define CPUID_VMX_BIT (1 << 5)

// VMCS Fields (Partial list for now)
#define HOST_CR0                    0x6C00
#define HOST_CR3                    0x6C02
#define HOST_CR4                    0x6C04
#define HOST_RSP                    0x6C14
#define HOST_RIP                    0x6C16
#define HOST_CS_SELECTOR            0x0C02
#define HOST_SS_SELECTOR            0x0C04
#define HOST_DS_SELECTOR            0x0C06
#define HOST_ES_SELECTOR            0x0C00
#define HOST_FS_SELECTOR            0x0C08
#define HOST_GS_SELECTOR            0x0C0A
#define HOST_TR_SELECTOR            0x0C0C
#define HOST_GDTR_BASE              0x6C0C
#define HOST_IDTR_BASE              0x6C0E

#define GUEST_CR0                   0x6800
#define GUEST_CR3                   0x6802
#define GUEST_CR4                   0x6804
#define GUEST_DR7                   0x681A
#define GUEST_RSP                   0x681C
#define GUEST_RIP                   0x681E
#define GUEST_RFLAGS                0x6820
#define GUEST_CS_SELECTOR           0x0802
#define GUEST_SS_SELECTOR           0x0804
#define GUEST_DS_SELECTOR           0x0806
#define GUEST_ES_SELECTOR           0x0800
#define GUEST_FS_SELECTOR           0x0808
#define GUEST_GS_SELECTOR           0x080A
#define GUEST_LDTR_SELECTOR         0x080C
#define GUEST_TR_SELECTOR           0x080E
#define GUEST_GDTR_BASE             0x6816
#define GUEST_IDTR_BASE             0x6818
#define GUEST_GDTR_LIMIT            0x4810
#define GUEST_IDTR_LIMIT            0x4812
#define GUEST_CS_LIMIT              0x4802
#define GUEST_CS_AR_BYTES           0x4816
#define GUEST_SS_LIMIT              0x4804
#define GUEST_SS_AR_BYTES           0x4818
#define GUEST_DR7                   0x681A

// EPT Pointer
#define EPT_POINTER                 0x201A
#define EPT_POINTER_HIGH            0x201B

#define VM_INSTRUCTION_ERROR        0x4400
#define VM_EXIT_REASON              0x4402
#define VM_EXIT_INSTRUCTION_LEN     0x440C
#define VM_EXIT_QUALIFICATION       0x6400

// Control fields
#define PIN_BASED_VM_EXEC_CONTROL   0x4000
#define CPU_BASED_VM_EXEC_CONTROL   0x4002
#define SECONDARY_VM_EXEC_CONTROL   0x401E
#define VM_EXIT_CONTROLS            0x400C
#define VM_ENTRY_CONTROLS           0x4012
#define VM_EXIT_MSR_STORE_COUNT     0x400E
#define VM_EXIT_MSR_LOAD_COUNT      0x4010
#define VM_ENTRY_MSR_LOAD_COUNT     0x4014

// Control bits
#define CPU_BASED_ACTIVATE_SECONDARY_CONTROLS (1 << 31)
#define CPU_BASED_HLT_EXITING       (1 << 7)
#define SECONDARY_EXEC_ENABLE_EPT   (1 << 1)
#define SECONDARY_EXEC_UNRESTRICTED_GUEST (1 << 7)

// Structure for a VMX region (VMXON or VMCS)
// Must be 4KB aligned
typedef struct {
    uint32_t revision_id;
    uint32_t abort_indicator;
    uint8_t data[4096 - 8];
} __attribute__((packed, aligned(4096))) VmxRegion;

void init_vmx();
int check_vmx_support();
void enable_vmx_in_cr4();

// External assembly functions
extern void vmx_vmxon(uint32_t phys_addr_low, uint32_t phys_addr_high);
extern void vmx_vmxoff();
extern void vmx_vmptrld(uint32_t phys_addr_low, uint32_t phys_addr_high);
extern void vmx_vmlaunch();
extern void vmx_vmresume();
extern uint32_t vmx_vmread(uint32_t field);
extern void vmx_vmwrite(uint32_t field, uint32_t value);

#endif
