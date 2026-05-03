#include "init_stages.h"
#include <arch/hardware/fpu.h>
#include <arch/hardware/lapic.h>
#include <arch/hardware/tsc.h>
#include <arch/internal/cpuid.h>
#include <common/arch.h>
#include <common/interrupts/dw.h>
#include <common/interrupts/interrupt.h>
#include <common/time.h>
#include <lib/log.h>

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
