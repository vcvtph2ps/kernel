#include <arch/hardware/i8042/i8042.h>
#include <arch/hardware/ioapic.h>
#include <common/arch.h>
#include <common/boot/bootloader.h>
#include <common/cpu_local.h>
#include <common/init.h>
#include <lib/log.h>
#include <memory/vm.h>
#include <uacpi/uacpi.h>

void init_stage_acpi_bootstrap(uint32_t core_id) {
    if(!INIT_CORE_IS_BSP(core_id)) {
        return;
    }
    void* temp_buffer = vm_map_anon(g_vm_global_address_space, VM_NO_HINT, 4096, VM_PROT_RW, VM_CACHE_NORMAL, VM_FLAG_ZERO);
    uacpi_status ret = uacpi_setup_early_table_access(temp_buffer, 4096);
    if(uacpi_unlikely_error(ret)) {
        arch_panic("uacpi_setup_early_table_access error: %s", uacpi_status_to_string(ret));
    }
}

void init_stage_arch_platform(uint32_t core_id) {
    if(!INIT_CORE_IS_BSP(core_id)) {
        return;
    }
    arch_ioapic_init_bsp();
    arch_i8042_init();
}

void init_stage_arch_ap_init(uint32_t core_id) {
    if(!INIT_CORE_IS_BSP(core_id)) {
        return;
    }
    cpu_local_init_storage(g_bootloader_info.cpu_count);
    uint32_t current_core_id = 1;
    for(size_t i = 0; i < g_bootloader_info.cpu_count; i++) {
        bootloader_cpu_info_t cpu_info;
        if(!bootloader_get_cpu(i, &cpu_info)) {
            arch_panic("Failed to get cpu info for cpu %zu\n", i);
        }
        if(cpu_info.is_bsp) {
            continue;
        }
        if(!cpu_info.can_boot) {
            continue;
        }
        ATOMIC_STORE(&g_init_ap_finished, 0, ATOMIC_RELAXED);
        LOG_OKAY("Starting AP with core id %u\n", current_core_id);
        bootloader_start_ap(i, current_core_id);
        while(ATOMIC_LOAD(&g_init_ap_finished, ATOMIC_ACQUIRE) == 0) {
            arch_relax();
        }
        current_core_id++;
    }
}
