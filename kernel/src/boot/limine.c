#include <assert.h>
#include <common/arch.h>
#include <common/boot/bootloader.h>
#include <limine.h>
#include <log.h>
#include <stddef.h>
#include <stdint.h>

#define LIMINE_REQUEST [[gnu::used, gnu::section(".limine_requests")]]

LIMINE_REQUEST volatile struct limine_framebuffer_request g_framebuffer_request = { .id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0 };
LIMINE_REQUEST volatile struct limine_hhdm_request g_hhdm_request = { .id = LIMINE_HHDM_REQUEST_ID, .revision = 0 };
LIMINE_REQUEST volatile struct limine_memmap_request g_memmap_request = { .id = LIMINE_MEMMAP_REQUEST_ID, .revision = 0 };
LIMINE_REQUEST volatile struct limine_executable_address_request g_kernel_mapping = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID,
    .revision = 0,
};

#ifdef __ARCH_X86_64__
LIMINE_REQUEST volatile struct limine_mp_request g_mp_request = { .id = LIMINE_MP_REQUEST_ID, .revision = 0, .response = nullptr, .flags = LIMINE_MP_REQUEST_X86_64_X2APIC };
#else
LIMINE_REQUEST volatile struct limine_mp_request g_mp_request = { .id = LIMINE_MP_REQUEST_ID, .revision = 0, .response = nullptr, .flags = 0 };
#endif

LIMINE_REQUEST volatile struct limine_rsdp_request g_rsdp_request = {
    .id = LIMINE_RSDP_REQUEST_ID,
    .revision = 0,
};

LIMINE_REQUEST volatile struct limine_internal_module g_initramfs = {
    .path = "initramfs.rdk",
    .string = "initramfs.rdk",
    .flags = LIMINE_INTERNAL_MODULE_REQUIRED,
};

LIMINE_REQUEST volatile struct limine_internal_module* g_modules[] = { &g_initramfs };

LIMINE_REQUEST volatile struct limine_module_request g_module_request = { .id = LIMINE_MODULE_REQUEST_ID, .revision = 1, .internal_modules = (struct limine_internal_module**) &g_modules, .internal_module_count = 1 };

LIMINE_REQUEST volatile uint64_t g_limine_base_revision[] = LIMINE_BASE_REVISION(LIMINE_API_REVISION);
[[gnu::used, gnu::section(".limine_requests_start")]] volatile uint64_t g_limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;
[[gnu::used, gnu::section(".limine_requests_end")]] volatile uint64_t g_limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

bool bootloader_limine_get_framebuffer_info(size_t index, bootloader_framebuffer_info_t* info) {
    if(g_framebuffer_request.response == nullptr || index >= g_framebuffer_request.response->framebuffer_count) { return false; }
    struct limine_framebuffer* fb = g_framebuffer_request.response->framebuffers[index];
    info->vaddr = (virt_addr_t) fb->address;
    info->paddr = FROM_HHDM(fb->address);
    info->width = fb->width;
    info->height = fb->height;
    info->pitch = fb->pitch;
    info->bpp = fb->bpp;
    info->framebuffer_length = fb->width * fb->height * (fb->bpp / 8);
    info->masks.red_mask_size = fb->red_mask_size;
    info->masks.red_mask_shift = fb->red_mask_shift;
    info->masks.green_mask_size = fb->green_mask_size;
    info->masks.green_mask_shift = fb->green_mask_shift;
    info->masks.blue_mask_size = fb->blue_mask_size;
    info->masks.blue_mask_shift = fb->blue_mask_shift;
    return true;
}

bool bootloader_limine_get_mmap_entry(size_t index, bootloader_mmap_entry_t* info) {
    if(g_memmap_request.response == nullptr || index >= g_memmap_request.response->entry_count) { return false; }
    struct limine_memmap_entry* entry = g_memmap_request.response->entries[index];
    info->base = entry->base;
    info->length = entry->length;
    switch(entry->type) {
        case LIMINE_MEMMAP_USABLE:                 info->type = BOOTLOADER_MMAP_USABLE; break;
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE: info->type = BOOTLOADER_MMAP_RESERVED_MUST_MAP; break;
        case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES: info->type = BOOTLOADER_MMAP_RESERVED_MUST_MAP; break;

        case LIMINE_MEMMAP_ACPI_RECLAIMABLE: [[fallthrough]];
        case LIMINE_MEMMAP_ACPI_NVS:         [[fallthrough]];
        case LIMINE_MEMMAP_BAD_MEMORY:       [[fallthrough]];
        case LIMINE_MEMMAP_FRAMEBUFFER:      [[fallthrough]];
        case LIMINE_MEMMAP_RESERVED:         [[fallthrough]];
        case LIMINE_MEMMAP_RESERVED_MAPPED:  info->type = BOOTLOADER_MMAP_RESERVED; break;
        default:                             return false;
    }
    return true;
}

bool bootloader_limine_get_cpu_info(size_t index, bootloader_cpu_info_t* info) {
    if(g_mp_request.response == nullptr || index >= g_mp_request.response->cpu_count) { return false; }
    struct limine_mp_info* cpu = g_mp_request.response->cpus[index];
    info->can_boot = true;
    info->is_bsp = cpu->lapic_id == g_mp_request.response->bsp_lapic_id;
    return true;
}

void limine_ap_start(struct limine_mp_info* info) {
    arch_init_ap(info->extra_argument);
    ASSERT_UNREACHABLE();
}

bool bootloader_limine_start_ap(size_t index, uint64_t arg) {
    if(g_mp_request.response == nullptr || index >= g_mp_request.response->cpu_count) { return false; }
    struct limine_mp_info* cpu = g_mp_request.response->cpus[index];
    assert(cpu->lapic_id != g_mp_request.response->bsp_lapic_id && "cannot start bsp");

    cpu->extra_argument = arg;
    __atomic_store_n(&cpu->goto_address, limine_ap_start, __ATOMIC_RELAXED);
    return true;
}

bool bootloader_limine_get_module(size_t index, bootloader_module_t* module) {
    if(g_module_request.response == nullptr || index >= g_module_request.response->module_count) { return false; }
    struct limine_file* file = g_module_request.response->modules[index];
    module->path = file->path;
    module->address = file->address;
    module->size = file->size;
    return true;
}


void bootloader_limine_set_framebuffer_address(size_t i, void* address) {
    if(g_framebuffer_request.response == nullptr || i >= g_framebuffer_request.response->framebuffer_count) { return; }
    g_framebuffer_request.response->framebuffers[i]->address = address;
}

// #define MAP_SEGMENT(name, map_type)                                                                                                                                                                                             \
//     {                                                                                                                                                                                                                           \
//         extern char name##_start[];                                                                                                                                                                                             \
//         extern char name##_end[];                                                                                                                                                                                               \
//         uintptr_t offset = name##_start - kernel_start;                                                                                                                                                                         \
//         uintptr_t size = name##_end - name##_start;                                                                                                                                                                             \
//         LOG_INFO("%s - 0x%llx, 0x%llx\n", #name, offset, size);                                                                                                                                                                 \
//         for(uintptr_t i = offset; i < offset + size; i += PAGE_SIZE_DEFAULT) { ptm_map(g_vm_global_address_space, kernel_virt + i, kernel_phys + i, PAGE_SIZE_DEFAULT, map_type, VM_CACHE_NORMAL, VM_PRIVILEGE_KERNEL, true); } \
//     }

void bootloader_limine_map_kernel_segments() {
    // extern char kernel_start[];
    // extern char kernel_end[];
    // phys_addr_t kernel_phys = (phys_addr_t) g_kernel_mapping.response->physical_base;
    // virt_addr_t kernel_virt = (virt_addr_t) kernel_start;

    // MAP_SEGMENT(text, VM_PROT_RX);
    // MAP_SEGMENT(rodata, VM_PROT_RO);
    // MAP_SEGMENT(data, VM_PROT_RW);
    // MAP_SEGMENT(requests, VM_PROT_RW);
}

void kmain_limine(void) {
    if(!LIMINE_BASE_REVISION_SUPPORTED(g_limine_base_revision)) { arch_die(); }
    log_print_nolock(LOG_LEVEL_OKAY, "limine | protocol revision %d\n", LIMINE_API_REVISION);

    g_bootloader_info.type = BOOTLOADER_LIMINE;
    g_bootloader_info.hhdm_offset = g_hhdm_request.response->offset;
    g_bootloader_info.rdsp_base = FROM_HHDM(g_rsdp_request.response->address);
    g_bootloader_info.mmap_entry_count = g_memmap_request.response->entry_count;
    g_bootloader_info.framebuffer_count = g_framebuffer_request.response ? g_framebuffer_request.response->framebuffer_count : 0;
    g_bootloader_info.cpu_count = g_mp_request.response ? g_mp_request.response->cpu_count : 1;
    g_bootloader_info.module_count = g_module_request.response ? g_module_request.response->module_count : 0;

    g_bootloader_info.internal_get_framebuffer_info = bootloader_limine_get_framebuffer_info;
    g_bootloader_info.internal_get_mmap_entry = bootloader_limine_get_mmap_entry;
    g_bootloader_info.internal_map_kernel_segments = bootloader_limine_map_kernel_segments;
    g_bootloader_info.internal_set_framebuffer_address = bootloader_limine_set_framebuffer_address;
    g_bootloader_info.internal_get_cpu_info = bootloader_limine_get_cpu_info;
    g_bootloader_info.internal_get_module = bootloader_limine_get_module;
    g_bootloader_info.internal_start_ap = bootloader_limine_start_ap;

    arch_init_bsp();
    arch_die();
}
