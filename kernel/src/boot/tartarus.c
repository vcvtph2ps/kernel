#include <assert.h>
#include <common/arch.h>
#include <common/boot/bootloader.h>
#include <log.h>
#include <memory/memory.h>
#include <tartarus.h>

tartarus_boot_info_t* g_tartarus_boot_info;

bool bootloader_tartarus_get_framebuffer_info(size_t index, bootloader_framebuffer_info_t* info) {
    if(index >= g_tartarus_boot_info->framebuffer_count) { return false; }

    tartarus_framebuffer_t fb = g_tartarus_boot_info->framebuffers[index];
    info->vaddr = (virt_addr_t) fb.vaddr;
    info->paddr = fb.paddr;
    info->width = fb.width;
    info->height = fb.height;
    info->pitch = fb.pitch;
    info->bpp = fb.bpp;
    info->framebuffer_length = fb.size;
    info->masks.red_mask_size = fb.mask.red_size;
    info->masks.red_mask_shift = fb.mask.red_position;
    info->masks.green_mask_size = fb.mask.green_size;
    info->masks.green_mask_shift = fb.mask.green_position;
    info->masks.blue_mask_size = fb.mask.blue_size;
    info->masks.blue_mask_shift = fb.mask.blue_position;
    return true;
}

bool bootloader_tartarus_get_mmap_entry(size_t index, bootloader_mmap_entry_t* info) {
    if(index >= g_tartarus_boot_info->mm_entry_count) { return false; }
    tartarus_mm_entry_t entry = g_tartarus_boot_info->mm_entries[index];
    info->base = entry.base;
    info->length = entry.length;
    switch(entry.type) {
        case TARTARUS_MM_TYPE_USABLE: info->type = BOOTLOADER_MMAP_USABLE; break;

        case TARTARUS_MM_TYPE_EFI_RECLAIMABLE:        [[fallthrough]];
        case TARTARUS_MM_TYPE_ACPI_RECLAIMABLE:       [[fallthrough]];
        case TARTARUS_MM_TYPE_BOOTLOADER_RECLAIMABLE: info->type = BOOTLOADER_MMAP_RESERVED_MUST_MAP; break;

        case TARTARUS_MM_TYPE_ACPI_NVS: [[fallthrough]];
        case TARTARUS_MM_TYPE_RESERVED: [[fallthrough]];
        case TARTARUS_MM_TYPE_BAD:      info->type = BOOTLOADER_MMAP_RESERVED; break;
        default:                        return false;
    }
    return true;
}

bool bootloader_tartarus_get_cpu(size_t index, bootloader_cpu_info_t* info) {
    if(index >= g_tartarus_boot_info->cpu_count) { return false; }
    tartarus_cpu_t cpu = g_tartarus_boot_info->cpus[index];
    info->can_boot = (cpu.flags & TARTARUS_CPU_FLAG_BOOT_OK) != 0;
    info->is_bsp = (cpu.flags & TARTARUS_CPU_FLAG_IS_BSP) != 0;
    return true;
}

void tartarus_ap_start(uint64_t param) {
    arch_init_ap(param);
    arch_die();
}

bool bootloader_tartarus_start_ap(size_t index, uint64_t arg) {
    if(index >= g_tartarus_boot_info->cpu_count) { return false; }
    tartarus_cpu_t cpu = g_tartarus_boot_info->cpus[index];
    assert(((cpu.flags & TARTARUS_CPU_FLAG_BOOT_OK) != 0) && "cannot start ap");
    assert(((cpu.flags & TARTARUS_CPU_FLAG_IS_BSP) == 0) && "cannot start bsp");

    *cpu.argument = arg;
    __atomic_store_n(cpu.park_address, (virt_addr_t) &tartarus_ap_start, __ATOMIC_RELAXED);

    return true;
}


bool bootloader_tartarus_get_module(size_t index, bootloader_module_t* module) {
    if(index >= g_tartarus_boot_info->module_count) { return false; }
    tartarus_module_t file = g_tartarus_boot_info->modules[index];
    module->path = file.name;
    module->address = (void*) TO_HHDM(file.paddr);
    module->size = file.size;
    return true;
}


void bootloader_tartarus_set_framebuffer_address(size_t i, void* address) {
    if(i >= g_tartarus_boot_info->framebuffer_count) { return; }
    g_tartarus_boot_info->framebuffers[i].vaddr = address;
}

void bootloader_tartarus_map_kernel_segments() {
    // for(size_t i = 0; i < g_tartarus_boot_info->kernel_segment_count; i++) {
    //     tartarus_kernel_segment_t segment = g_tartarus_boot_info->kernel_segments[i];
    //     vm_protection_t flags = VM_PROT_NO_ACCESS;
    //     if(segment.flags & TARTARUS_KERNEL_SEGMENT_FLAG_READ) { flags.read = true; }
    //     if(segment.flags & TARTARUS_KERNEL_SEGMENT_FLAG_WRITE) { flags.write = true; }
    //     if(segment.flags & TARTARUS_KERNEL_SEGMENT_FLAG_EXECUTE) { flags.execute = true; }
    //     printf("0x%llx -> 0x%llx - %zu [%c%c%c]\n", segment.paddr, segment.vaddr, segment.size, flags.read ? 'r' : '-', flags.write ? 'w' : '-', flags.execute ? 'x' : '-');
    //     for(uintptr_t i = 0; i < segment.size; i += PAGE_SIZE_DEFAULT) { ptm_map(g_vm_global_address_space, segment.vaddr + i, segment.paddr + i, PAGE_SIZE_DEFAULT, flags, VM_CACHE_NORMAL, VM_PRIVILEGE_KERNEL, true); }
    // }
}


void kmain_tartarus(tartarus_boot_info_t* boot_info, uint16_t version) {
    uint8_t major = version >> 8;
    uint8_t minor = version & 0xff;

    log_print_nolock(LOG_LEVEL_OKAY, "tartarus | protocol version: %d.%d\n", major, minor);
    assert(major == 2 && minor >= 0);

    log_print_nolock(LOG_LEVEL_INFO, "Framebuffer count %d\n", boot_info->framebuffer_count);
    log_print_nolock(LOG_LEVEL_INFO, "AP count %d\n", boot_info->cpu_count - 1);

    g_tartarus_boot_info = boot_info;

    g_bootloader_info.type = BOOTLOADER_TARTARUS;
    g_bootloader_info.hhdm_offset = boot_info->hhdm_offset;
    g_bootloader_info.rdsp_base = boot_info->acpi_rsdp_address;
    g_bootloader_info.mmap_entry_count = boot_info->mm_entry_count;
    g_bootloader_info.cpu_count = boot_info->cpu_count;
    g_bootloader_info.module_count = boot_info->module_count;
    g_bootloader_info.framebuffer_count = boot_info->framebuffer_count;

    g_bootloader_info.internal_get_framebuffer_info = bootloader_tartarus_get_framebuffer_info;
    g_bootloader_info.internal_get_mmap_entry = bootloader_tartarus_get_mmap_entry;
    g_bootloader_info.internal_map_kernel_segments = bootloader_tartarus_map_kernel_segments;
    g_bootloader_info.internal_set_framebuffer_address = bootloader_tartarus_set_framebuffer_address;
    g_bootloader_info.internal_get_cpu_info = bootloader_tartarus_get_cpu;
    g_bootloader_info.internal_get_module = bootloader_tartarus_get_module;
    g_bootloader_info.internal_start_ap = bootloader_tartarus_start_ap;

    arch_init_bsp();
    arch_die();
}
