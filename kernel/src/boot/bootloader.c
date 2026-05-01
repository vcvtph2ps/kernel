#include <assert.h>
#include <common/boot/bootloader.h>
#include <string.h>

bootloader_info_t g_bootloader_info;

bool bootloader_get_framebuffer(size_t index, bootloader_framebuffer_info_t* info) {
    return g_bootloader_info.internal_get_framebuffer_info(index, info);
}

bool bootloader_get_mmap_entry(size_t index, bootloader_mmap_entry_t* info) {
    return g_bootloader_info.internal_get_mmap_entry(index, info);
}

bool bootloader_get_cpu(size_t index, bootloader_cpu_info_t* info) {
    return g_bootloader_info.internal_get_cpu_info(index, info);
}

bool bootloader_get_module(size_t index, bootloader_module_t* info) {
    return g_bootloader_info.internal_get_module(index, info);
}

bool bootloader_find_module(const char* path, bootloader_module_t* out_module) {
    for(size_t i = 0; i < g_bootloader_info.module_count; i++) {
        bootloader_module_t module = {};
        if(!bootloader_get_module(i, &module)) {
            assert(false);
            return false;
        }
        if(strcmp(module.path, path) == 0) { return bootloader_get_module(i, out_module); }
    }
    return false;
}

bool bootloader_start_ap(size_t index, uint64_t arg) {
    return g_bootloader_info.internal_start_ap(index, arg);
}

void bootloader_map_kernel_segments() {
    g_bootloader_info.internal_map_kernel_segments();
}

void bootloader_set_framebuffer_address(size_t i, void* address) {
    g_bootloader_info.internal_set_framebuffer_address(i, address);
}

const char* bootloader_memmap_type_to_str(bootloader_mmap_entry_type_t type) {
    switch(type) {
        case BOOTLOADER_MMAP_USABLE:            return "usable";
        case BOOTLOADER_MMAP_RESERVED:          return "reserved";
        case BOOTLOADER_MMAP_RESERVED_MUST_MAP: return "reserved map";
        default:                                return "unknown";
    }
}

const char* bootloader_type_to_str(bootloader_type_t type) {
    switch(type) {
        case BOOTLOADER_LIMINE:   return "limine";
        case BOOTLOADER_TARTARUS: return "tartarus";
        default:                  return "unknown";
    }
}
