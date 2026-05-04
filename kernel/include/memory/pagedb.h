#pragma once
#include <stddef.h>
#include <stdint.h>

#include "memory/memory.h"

typedef struct pagedb_page pagedb_page_t;

struct [[gnu::aligned(64)]] pagedb_page {
    uint32_t ref_count;
};

/**
 * @brief early pagedb init called before ptm_init_kernel.
 * @note g_pagedb_db is still NULL after this call.
 */
void pagedb_init_early();

/**
 * @brief late pagedb init
 * @param virt_base   Virtual address at which the array has been mapped.
 * @param byte_size   Size of the mapping in bytes (page-aligned).
 */
void pagedb_init_late(virt_addr_t virt_base, size_t byte_size);

/**
 * @brief Number of physical pages tracked
 * @note valid after pagedb_init_early().
 */
size_t pagedb_page_count();

/**
 * @brief Byte size of the pagedb array
 * @note valid after pagedb_init_early().
 */
size_t pagedb_byte_size();

/**
 * @brief Returns the pagedb entry for a physical address, or NULL if the
 * db is not yet live or the address exceeds the tracked range.
 */
pagedb_page_t* pagedb_get_for_phys(phys_addr_t paddr);
