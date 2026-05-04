#include <common/boot/bootloader.h>
#include <log.h>
#include <memory/memory.h>
#include <memory/pagedb.h>

pagedb_page_t* g_pagedb_db;
static size_t g_pagedb_page_count;
static size_t g_pagedb_array_bytes;

void pagedb_init_early() {
    phys_addr_t phys_top = 0;

    for(size_t i = 0; i < g_bootloader_info.mmap_entry_count; i++) {
        bootloader_mmap_entry_t entry;
        if(!bootloader_get_mmap_entry(i, &entry)) continue;

        phys_addr_t entry_top = ALIGN_UP(entry.base + entry.length, PAGE_SIZE_DEFAULT);
        if(entry_top > phys_top) phys_top = entry_top;
    }

    g_pagedb_page_count = phys_top / PAGE_SIZE_DEFAULT;
    g_pagedb_array_bytes = ALIGN_UP(g_pagedb_page_count * sizeof(pagedb_page_t), PAGE_SIZE_DEFAULT);

    LOG_OKAY("pagedb: tracking %zu pages, array %zu KiB\n", g_pagedb_page_count, g_pagedb_array_bytes / 1024);
}

void pagedb_init_late(virt_addr_t virt_base, size_t byte_size) {
    (void) byte_size;
    g_pagedb_db = (pagedb_page_t*) virt_base;
    LOG_OKAY("pagedb: live at 0x%lx (%zu KiB)\n", virt_base, g_pagedb_array_bytes / 1024);
}

size_t pagedb_page_count() {
    return g_pagedb_page_count;
}

size_t pagedb_byte_size() {
    return g_pagedb_array_bytes;
}

pagedb_page_t* pagedb_get_for_phys(phys_addr_t paddr) {
    if(g_pagedb_db == nullptr) return nullptr;
    size_t pfn = paddr / PAGE_SIZE_DEFAULT;
    if(pfn >= g_pagedb_page_count) return nullptr;
    return &g_pagedb_db[pfn];
}
