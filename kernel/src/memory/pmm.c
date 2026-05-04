#include <assert.h>
#include <common/arch.h>
#include <common/boot/bootloader.h>
#include <log.h>
#include <memory/memory.h>
#include <memory/pagedb.h>
#include <memory/pmm.h>
#include <spinlock.h>
#include <stddef.h>
#include <string.h>

typedef struct pmm_node pmm_node_t;

struct pmm_node {
    pmm_node_t* next;
};

static spinlock_no_dw_t g_pmm_lock = SPINLOCK_NO_DW_INIT;
static pmm_node_t* g_pmm_head;

void pmm_init(void) {
    for(size_t i = 0; i < g_bootloader_info.mmap_entry_count; i++) {
        bootloader_mmap_entry_t entry;
        if(!bootloader_get_mmap_entry(i, &entry)) {
            continue;
        }

        LOG_INFO("%s, 0x%016lx - 0x%016lx (%lx)\n", bootloader_memmap_type_to_str(entry.type), entry.base, entry.base + entry.length, entry.length);

        if(entry.type != BOOTLOADER_MMAP_USABLE) {
            continue;
        }

        assert((entry.base % PAGE_SIZE_DEFAULT) == 0);
        assert(((entry.base + entry.length) % PAGE_SIZE_DEFAULT) == 0);
        phys_addr_t start = entry.base;
        phys_addr_t end = entry.base + entry.length;

        for(phys_addr_t addr = start; addr < end; addr += PAGE_SIZE_DEFAULT) {
            pmm_node_t* node = (pmm_node_t*) TO_HHDM(addr);
            node->next = g_pmm_head;
            g_pmm_head = node;
        }
    }
}

[[nodiscard]] phys_addr_t pmm_alloc_page(pmm_flags_t flags) {
    spinlock_nodw_lock(&g_pmm_lock);
    pmm_node_t* current = g_pmm_head;
    if(current == nullptr) {
        spinlock_nodw_unlock(&g_pmm_lock);
        return 0;
    }

    g_pmm_head = g_pmm_head->next;
    spinlock_nodw_unlock(&g_pmm_lock);

    if(flags & PMM_FLAG_ZERO) {
        memset(current, 0, PAGE_SIZE_DEFAULT);
    }

    phys_addr_t paddr = (phys_addr_t) FROM_HHDM(current);

    pagedb_page_t* entry = pagedb_get_for_phys(paddr);
    if(entry != nullptr) {
        __atomic_fetch_add(&entry->ref_count, 1, __ATOMIC_RELAXED);
    }

    return paddr;
}

/**
 * @brief Frees a single page of physical memory.
 * @param addr The physical address of the page to free. Must be page-aligned.
 */
void pmm_free_page(phys_addr_t addr) {
    assert((addr % PAGE_SIZE_DEFAULT) == 0);

    pagedb_page_t* entry = pagedb_get_for_phys(addr);
    if(entry != nullptr) {
        uint32_t prev = __atomic_fetch_sub(&entry->ref_count, 1, __ATOMIC_RELAXED);
        if(prev > 1) return;
    }

    pmm_node_t* node = (pmm_node_t*) TO_HHDM(addr);

    spinlock_nodw_lock(&g_pmm_lock);
    node->next = g_pmm_head;
    g_pmm_head = node;
    spinlock_nodw_unlock(&g_pmm_lock);
}
