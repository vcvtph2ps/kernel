#include <arch/hardware/fpu.h>
#include <arch/hardware/lapic.h>
#include <arch/internal/cpuid.h>
#include <common/arch.h>
#include <common/boot/bootloader.h>
#include <common/cpu_local.h>
#include <common/interrupts/dw.h>
#include <common/interrupts/interrupt.h>
#include <lib/log.h>
#include <memory/heap.h>
#include <memory/memory.h>
#include <memory/pmm.h>
#include <memory/ptm.h>
#include <memory/slab.h>

static uint32_t g_arch_ap_finished = 0;

void init_aps() {
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

void arch_init_bsp() {
    cpu_local_init_bsp();
    pmm_init();
    ptm_init_kernel_bsp();
    log_init();

    slab_cache_init();
    heap_init();

    LOG_INFO("CPU Vendor: %s\n", arch_cpuid_get_vendor_string());
    LOG_INFO("CPU Name: %s\n", arch_cpuid_get_name_string());

    arch_gdt_init_common();
    LOG_OKAY("GDT INIT OKAY!\n");
    dw_init_bsp();
    interrupt_init_bsp();
    LOG_OKAY("INTERRUPT INIT OKAY!\n");
    arch_lapic_init_bsp();
    LOG_OKAY("LAPIC INIT OKAY!\n");

    arch_fpu_init_bsp();
    LOG_OKAY("FPU INIT OKAY!\n");


    cpu_local_init_storage(g_bootloader_info.cpu_count);
    init_aps();

    while(1);
}

void arch_init_ap(uint32_t core_id) {
    (void) core_id;
    ptm_init_kernel_ap();

    LOG_OKAY("core %u early init\n", core_id);

    cpu_local_init_ap(core_id);
    arch_lapic_init_ap_early();
    arch_gdt_init_common();
    interrupt_init_ap();
    arch_fpu_init_ap();

    LOG_OKAY("core %u done init\n", core_id);
    while(1);
}
