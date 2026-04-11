#pragma once

#include <memory/memory.h>

#define PMM_FLAG_NONE (0)
#define PMM_FLAG_ZERO (1 << 0)

typedef uint8_t pmm_flags_t;

/**
 * @brief Initializes the physical memory manager.
 */
void pmm_init(void);

/**
 * @brief Allocates a single page of physical memory.
 * @param flags Allocation flags. See PMM_FLAG_*.
 * @return The physical address of the allocated page, or 0 if allocation failed.
 */
[[nodiscard]] phys_addr_t pmm_alloc_page(pmm_flags_t flags);

/**
 * @brief Frees a single page of physical memory.
 * @param addr The physical address of the page to free. Must be page-aligned.
 */
void pmm_free_page(phys_addr_t addr);
