#include <arch/hardware/fpu.h>
#include <arch/hardware/i8042/i8042.h>
#include <arch/hardware/ioapic.h>
#include <arch/hardware/lapic.h>
#include <arch/hardware/tsc.h>
#include <arch/internal/cpuid.h>
#include <common/arch.h>
#include <common/boot/bootloader.h>
#include <common/cpu_local.h>
#include <common/interrupts/dw.h>
#include <common/interrupts/interrupt.h>
#include <common/sched/sched.h>
#include <common/time.h>
#include <common/tty.h>
#include <common/userspace/process.h>
#include <common/userspace/syscall.h>
#include <fs/vfs.h>
#include <ldr/elfldr.h>
#include <ldr/sysv.h>
#include <lib/log.h>
#include <memory/heap.h>
#include <memory/memory.h>
#include <memory/pmm.h>
#include <memory/ptm.h>
#include <memory/slab.h>
#include <memory/vm.h>
#include <uacpi/uacpi.h>

#include "helpers.h"

typedef enum {
    INIT_STAGE_BASE_MEM,
    INIT_STAGE_ARCH_CPU,
    INIT_STAGE_TIME,
    INIT_STAGE_ACPI_BOOTSTRAP,
    INIT_STAGE_ARCH_PLATFORM,
    INIT_STAGE_ARCH_AP_INIT,
    INIT_STAGE_ARCH_USERSPACE,
    INIT_STAGE_VFS,
    INIT_STAGE_USERSPACE
} init_stage_t;

const char* init_stage_to_str(init_stage_t stage) {
    switch(stage) {
        case INIT_STAGE_BASE_MEM:       return "base_mem";
        case INIT_STAGE_ARCH_CPU:       return "arch_cpu";
        case INIT_STAGE_TIME:           return "time";
        case INIT_STAGE_ACPI_BOOTSTRAP: return "acpi_bootstrap";
        case INIT_STAGE_ARCH_PLATFORM:  return "arch_platform";
        case INIT_STAGE_ARCH_AP_INIT:   return "arch_ap_init";
        case INIT_STAGE_ARCH_USERSPACE: return "arch_userspace";
        case INIT_STAGE_VFS:            return "vfs";
        case INIT_STAGE_USERSPACE:      return "userspace";
    }
    __builtin_unreachable();
}

typedef struct {
    init_stage_t stage;
    void (*handler)(uint32_t core_id);
    bool bsp_only;
} init_stage_handler_t;

ATOMIC static uint32_t g_arch_bsp_finished = 0;
ATOMIC static uint32_t g_arch_ap_finished = 0;

void fn_init_stage_base_mem_bsp(uint32_t core_id) {
    (void) core_id;
    cpu_local_init_bsp();
    pmm_init();
    ptm_init_kernel_bsp();
    log_init();
    slab_cache_init();
    heap_init();
}

void fn_init_stage_base_mem_ap(uint32_t core_id) {
    ptm_init_kernel_ap();
    cpu_local_init_ap(core_id);

    LOG_OKAY("core %u early init\n", core_id);
}

void fn_init_stage_arch_cpu_bsp(uint32_t core_id) {
    (void) core_id;
    LOG_INFO("CPU Vendor: %s\n", arch_cpuid_get_vendor_string());
    LOG_INFO("CPU Name: %s\n", arch_cpuid_get_name_string());

    arch_gdt_init_common();
    dw_init_bsp();
    interrupt_init_bsp();
    arch_lapic_init_bsp();
    arch_fpu_init_bsp();
}

void fn_init_stage_arch_cpu_ap(uint32_t core_id) {
    (void) core_id;
    arch_lapic_init_ap_early();

    arch_gdt_init_common();
    interrupt_init_ap();
    arch_fpu_init_ap();
}

void fn_init_stage_time_bsp(uint32_t core_id) {
    (void) core_id;
    // @todo: move lapic timer here and use hpet for calibration instead of pit
    arch_tsc_calibrate();
    time_init();
}

void fn_init_stage_time_ap(uint32_t core_id) {
    (void) core_id;
    // @todo: move lapic timer here and use hpet for calibration instead of pit
    arch_tsc_calibrate();
}

void fn_init_stage_acpi_bootstrap(uint32_t core_id) {
    (void) core_id;
    void* temp_buffer = vm_map_anon(g_vm_global_address_space, VM_NO_HINT, 4096, VM_PROT_RW, VM_CACHE_NORMAL, VM_FLAG_ZERO);
    uacpi_status ret = uacpi_setup_early_table_access(temp_buffer, 4096);
    if(uacpi_unlikely_error(ret)) { arch_panic("uacpi_setup_early_table_access error: %s", uacpi_status_to_string(ret)); }
}

void fn_init_stage_arch_platform(uint32_t core_id) {
    (void) core_id;
    arch_ioapic_init_bsp();
    arch_i8042_init();
}

void fn_init_stage_arch_ap_init(uint32_t core_id) {
    (void) core_id;
    cpu_local_init_storage(g_bootloader_info.cpu_count);
    uint32_t current_core_id = 1;
    for(size_t i = 0; i < g_bootloader_info.cpu_count; i++) {
        bootloader_cpu_info_t cpu_info;
        if(!bootloader_get_cpu(i, &cpu_info)) { arch_panic("Failed to get cpu info for cpu %zu\n", i); }
        if(cpu_info.is_bsp) { continue; }
        if(!cpu_info.can_boot) { continue; }
        ATOMIC_STORE(&g_arch_ap_finished, 0, ATOMIC_RELAXED);
        LOG_OKAY("Starting AP with core id %u\n", current_core_id);
        bootloader_start_ap(i, current_core_id);
        while(ATOMIC_LOAD(&g_arch_ap_finished, ATOMIC_ACQUIRE) == 0) { arch_relax(); }
        current_core_id++;
    }
}

void fn_init_stage_arch_userspace(uint32_t core_id) {
    (void) core_id;
    g_tty = tty_init();
    syscall_init();
    sched_init_bsp();
}

void fn_init_stage_vfs(uint32_t core_id) {
    (void) core_id;
    for(size_t i = 0; i < g_bootloader_info.module_count; i++) {
        bootloader_module_t module = {};
        if(!bootloader_get_module(i, &module)) {
            assert(false);
            continue;
        }
        LOG_INFO("Module %zu: %s, address: %p, size: %zu\n", i, module.path, module.address, module.size);
    }

    bootloader_module_t initramfs_module;
    bool found_initramfs = bootloader_find_module("/boot/initramfs.rdk", &initramfs_module);
    if(!found_initramfs) { arch_panic("Failed to find initramfs\n"); }

    vfs_result_t res = vfs_mount(&g_vfs_rdsk_ops, nullptr, (void*) initramfs_module.address);
    if(res != VFS_RESULT_OK) { arch_panic("Failed to mount initramfs (%d)\n", res); }
    LOG_OKAY("mounted initramfs\n");

    vfs_node_t* root_node;
    res = vfs_root(&root_node);
    if(res != VFS_RESULT_OK) { arch_panic("Failed to get root node (%d)\n", res); }

    res = vfs_mount(&g_vfs_devfs_ops, &VFS_MAKE_ABS_PATH("/dev"), nullptr);
    if(res != VFS_RESULT_OK) { arch_panic("Failed to mount devfs (%d)\n", res); }
}

void fn_init_stage_userspace(uint32_t core_id) {
    (void) core_id;
    vm_address_space_t* process_as = heap_alloc(sizeof(vm_address_space_t));
    ptm_init_user(process_as);

    elfldr_elf_loader_info_t* elf_info;
    bool loaded_elf = elfldr_load_file(process_as, &VFS_MAKE_ABS_PATH("/usr/bin/bash"), &elf_info);
    assert(loaded_elf && "Failed to load init file");

    g_tty = tty_init();

    size_t stack_virt_size = 1024 * PAGE_SIZE_DEFAULT;
    virt_addr_t user_stack = (virt_addr_t) vm_map_anon(process_as, (void*) (MEMORY_USERSPACE_END - (10 * PAGE_SIZE_DEFAULT) - stack_virt_size), stack_virt_size, VM_PROT_RW, VM_CACHE_NORMAL, VM_FLAG_FIXED | VM_FLAG_ZERO | VM_FLAG_DYNAMICALLY_BACKED);
    virt_addr_t user_rsp = sysv_user_stack_init(process_as, user_stack + stack_virt_size, elf_info);
    process_t* process = process_create(process_as, elf_info->executable_entry_point, user_rsp);

    // @todo: what the fuck
    sched_thread_schedule(CONTAINER_OF(process->threads.head, thread_t, list_node_proc));
}

#define INIT_STAGE(STAGE, HANDLER, BSP_ONLY) { .stage = (STAGE), .handler = (HANDLER), .bsp_only = (BSP_ONLY) }

init_stage_handler_t g_init_stage_handlers[] = {
    INIT_STAGE(INIT_STAGE_BASE_MEM, fn_init_stage_base_mem_bsp, true),
    INIT_STAGE(INIT_STAGE_BASE_MEM, fn_init_stage_base_mem_ap, false),

    INIT_STAGE(INIT_STAGE_ARCH_CPU, fn_init_stage_arch_cpu_bsp, true),
    INIT_STAGE(INIT_STAGE_ARCH_CPU, fn_init_stage_arch_cpu_ap, false),

    INIT_STAGE(INIT_STAGE_TIME, fn_init_stage_time_bsp, true),
    INIT_STAGE(INIT_STAGE_TIME, fn_init_stage_time_ap, false),

    INIT_STAGE(INIT_STAGE_ACPI_BOOTSTRAP, fn_init_stage_acpi_bootstrap, true),
    INIT_STAGE(INIT_STAGE_ARCH_PLATFORM, fn_init_stage_arch_platform, true),
    INIT_STAGE(INIT_STAGE_ARCH_AP_INIT, fn_init_stage_arch_ap_init, true),
    INIT_STAGE(INIT_STAGE_ARCH_USERSPACE, fn_init_stage_arch_userspace, true),
    INIT_STAGE(INIT_STAGE_VFS, fn_init_stage_vfs, true),
    INIT_STAGE(INIT_STAGE_USERSPACE, fn_init_stage_userspace, true),
};

#undef INIT_STAGE

bool should_run_stage(bool is_bsp, bool bsp_only) {
    return (is_bsp && bsp_only) || (!is_bsp && !bsp_only);
}

void run_stage(init_stage_t stage, uint32_t core_id, bool is_bsp) {
    for(size_t i = 0; i < sizeof(g_init_stage_handlers) / sizeof(init_stage_handler_t); i++) {
        if(g_init_stage_handlers[i].stage == stage) {
            if(!should_run_stage(is_bsp, g_init_stage_handlers[i].bsp_only)) { continue; }

            g_init_stage_handlers[i].handler(core_id);
            LOG_OKAY("finished stage %s (%i) on %s\n", init_stage_to_str(stage), stage, is_bsp ? "bsp" : "ap");
            return;
        }
    }
    LOG_WARN("No handler found for stage %s (%i)\n", init_stage_to_str(stage), stage);
}

void arch_init_bsp() {
    ATOMIC_STORE(&g_arch_bsp_finished, 0, ATOMIC_RELAXED);

    run_stage(INIT_STAGE_BASE_MEM, 0, true);
    run_stage(INIT_STAGE_ARCH_CPU, 0, true);
    run_stage(INIT_STAGE_TIME, 0, true);
    run_stage(INIT_STAGE_ACPI_BOOTSTRAP, 0, true);
    run_stage(INIT_STAGE_ARCH_PLATFORM, 0, true);
    run_stage(INIT_STAGE_ARCH_AP_INIT, 0, true);
    run_stage(INIT_STAGE_ARCH_USERSPACE, 0, true);
    run_stage(INIT_STAGE_VFS, 0, true);
    run_stage(INIT_STAGE_USERSPACE, 0, true);

    ATOMIC_STORE(&g_arch_bsp_finished, 1, ATOMIC_RELAXED);

    sched_arch_handoff();
}

void arch_init_ap(uint32_t core_id) {
    run_stage(INIT_STAGE_BASE_MEM, core_id, false);
    run_stage(INIT_STAGE_ARCH_CPU, core_id, false);
    run_stage(INIT_STAGE_TIME, core_id, false);
    run_stage(INIT_STAGE_ACPI_BOOTSTRAP, core_id, false);
    run_stage(INIT_STAGE_ARCH_PLATFORM, core_id, false);
    run_stage(INIT_STAGE_ARCH_AP_INIT, core_id, false);
    run_stage(INIT_STAGE_ARCH_USERSPACE, core_id, false);
    run_stage(INIT_STAGE_VFS, core_id, false);
    run_stage(INIT_STAGE_USERSPACE, core_id, false);

    ATOMIC_STORE(&g_arch_ap_finished, 1, ATOMIC_RELAXED);
    while(ATOMIC_LOAD(&g_arch_ap_finished, ATOMIC_ACQUIRE) == 0) { arch_relax(); }

    while(1);
}
