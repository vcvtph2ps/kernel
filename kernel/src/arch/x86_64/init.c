#include <arch/internal/cpuid.h>
#include <common/arch.h>
#include <common/cpu_local.h>
#include <lib/log.h>

void arch_init_bsp() {
    cpu_local_init_bsp();
    log_init();

    LOG_INFO("CPU Vendor: %s\n", arch_cpuid_get_vendor_string());
    LOG_INFO("CPU Name: %s\n", arch_cpuid_get_name_string());

    while(1);
}

void arch_init_ap(uint32_t core_id) {
    (void) core_id;
    while(1);
}
