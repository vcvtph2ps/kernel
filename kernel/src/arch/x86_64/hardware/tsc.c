#include <arch/cpu_local.h>
#include <arch/hardware/pit.h>
#include <log.h>
#include <stdint.h>

void arch_tsc_calibrate() {
    uint64_t tsc_start = __rdtsc();
    arch_pit_sleep_us(10000);
    uint32_t tsc_ticks_per_us = (__rdtsc() - tsc_start) / 10000ull;
    LOG_OKAY("tsc calibrated: %d ticks/us\n", tsc_ticks_per_us);
    CPU_LOCAL_WRITE(tsc_ticks_per_us, tsc_ticks_per_us);
}
