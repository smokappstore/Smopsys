#include "ept.h"
#include "../drivers/vga_holographic.h"
// #include "memory.h" // Assuming memory manager exists for page allocation

// For now, we will use static allocation for a simple identity map of the first 4GB
// We need:
// 1 PML4
// 4 PDPTs (covering 512GB each, but we only need 1 actually for 4GB, wait. 1 PML4 entry covers 512GB)
// So 1 PML4 -> 1 PDPT entry -> 1 PDPT table?
// VMX EPT is 4 levels.
// PML4 (512 entries) -> PDPT (512 entries) -> PD (512 entries) -> PT (512 entries)
// 1 PML4 Entry covers 512 * 1GB = 512GB.
// 1 PDPT Entry covers 1GB.
// 1 PD Entry covers 2MB.
// To map 4GB:
// We need 4 PDPT entries (4 * 1GB = 4GB).
// Each PDPT entry points to a Page Directory.
// So we need 4 Page Directories.
// Each PD has 512 entries. If using 2MB pages (Large Page), we don't need PTs.
// 512 * 2MB = 1GB.
// So 4 PDs full of 2MB entries covers 4GB.

// Total static size:
// 1 PML4 table
// 1 PDPT table
// 4 PD tables
// Total: 6 pages = 24KB.

static ept_table_t pml4;
static ept_table_t pdpt;
static ept_table_t pds[4];

// Helper to get physical address (assuming flat identity for kernel data)
static inline uint64_t get_phys(void *ptr) {
    return (uint64_t)(uint32_t)ptr;
}

void init_ept(ept_context_t *ctx) {
    // Clear tables
    for (int i = 0; i < 512; i++) {
        pml4.entries[i] = 0;
        pdpt.entries[i] = 0;
    }
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 512; i++) {
            pds[j].entries[i] = 0;
        }
    }
    
    ctx->pml4 = &pml4;
}

void ept_map_identity(ept_context_t *ctx) {
    // 1. Setup PML4 entry 0 to point to PDPT
    ctx->pml4->entries[0] = get_phys(&pdpt) | EPT_READ | EPT_WRITE | EPT_EXECUTE;
    
    // 2. Setup PDPT entries 0-3 to point to PDs 0-3
    for (int i = 0; i < 4; i++) {
        pdpt.entries[i] = get_phys(&pds[i]) | EPT_READ | EPT_WRITE | EPT_EXECUTE;
    }
    
    // 3. Setup PD entries for 2MB pages
    // We map 0 to 4GB Identity, but OFFSET to protect Host Kernel
    // Host Kernel is at 1MB+. We'll offset Guest Physical 0 to Host Physical 32MB (0x2000000)
    // This assumes Host has > 4GB + 32MB RAM.
    
    #define GUEST_MEMORY_OFFSET 0x2000000
    
    uint64_t phys_addr = GUEST_MEMORY_OFFSET;
    
    for (int pd_idx = 0; pd_idx < 4; pd_idx++) {
        for (int entry_idx = 0; entry_idx < 512; entry_idx++) {
            // 2MB Page, RWX, WriteBack
            pds[pd_idx].entries[entry_idx] = phys_addr 
                                           | EPT_READ | EPT_WRITE | EPT_EXECUTE 
                                           | EPT_LARGE_PAGE 
                                           | EPT_MEMORY_TYPE_WB;
            
            phys_addr += 0x200000; // +2MB
        }
    }
    
    vga_holographic_write("[EPT] Mapped 4GB Guest Phys -> Host Phys [32MB+]\n");
}

uint64_t ept_get_pointer(ept_context_t *ctx) {
    uint64_t ptr = get_phys(ctx->pml4);
    // Attributes for EPT Pointer (EPTP)
    // Bit 0-2: Memory type (6 = WB) (Wait, bits 0-2 are memory type dependent on MTRRs usually, but 6 is WB)
    // Bit 3-5: Page walk length - 1 (So 3 for 4 levels)
    // Bit 6: Dirty and Accessed enable (if supported) - keep 0 for now
    
    uint64_t eptp = ptr | (6) | (3 << 3);
    return eptp;
}
