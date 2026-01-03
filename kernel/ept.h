#ifndef EPT_H
#define EPT_H

#include <stdint.h>

// EPT Entry flags
#define EPT_READ        (1 << 0)
#define EPT_WRITE       (1 << 1)
#define EPT_EXECUTE     (1 << 2)
#define EPT_MEMORY_TYPE_WB  (6 << 3) // Write Back
#define EPT_IGNORE_PAT  (1 << 6)
#define EPT_LARGE_PAGE  (1 << 7) // 2MB page in PD, 1GB in PDPT

// Structure representing an EPT entry (64-bit)
typedef uint64_t ept_entry_t;

// EPT Table structures (512 entries each)
// Aligned to 4KB
typedef struct {
    ept_entry_t entries[512];
} __attribute__((packed, aligned(4096))) ept_table_t;

// Context for a VM's EPT
typedef struct {
    ept_table_t *pml4;
} ept_context_t;

// Functions
void init_ept(ept_context_t *ctx);
void ept_map_identity(ept_context_t *ctx);
uint64_t ept_get_pointer(ept_context_t *ctx);

#endif
