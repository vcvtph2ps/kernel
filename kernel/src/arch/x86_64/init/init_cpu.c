#include <arch/hardware/fpu.h>
#include <arch/hardware/lapic.h>
#include <arch/hardware/tsc.h>
#include <arch/internal/cpuid.h>
#include <arch/internal/gdt.h>
#include <common/arch.h>
#include <common/init.h>
#include <common/interrupts/dw.h>
#include <common/interrupts/interrupt.h>
#include <common/time.h>
#include <lib/log.h>

void init_stage_arch_cpu(uint32_t core_id) {
    if(INIT_CORE_IS_BSP(core_id)) {
        LOG_INFO("CPU Vendor: %s\n", arch_cpuid_get_vendor_string());
        LOG_INFO("CPU Name: %s\n", arch_cpuid_get_name_string());

        dw_init();
    }

    arch_gdt_init_common();
    interrupt_init(core_id);
    arch_lapic_init(core_id);
    arch_fpu_init(core_id);
}

void init_stage_time(uint32_t core_id) {
    (void) core_id;
    // @todo: move lapic timer here and use hpet for calibration instead of pit
    arch_tsc_calibrate();
    if(INIT_CORE_IS_BSP(core_id)) {
        time_init();
    }
}
