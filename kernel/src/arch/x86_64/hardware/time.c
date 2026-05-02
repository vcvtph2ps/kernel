#include <arch/cpu_local.h>
#include <common/boot/bootloader.h>
#include <common/time.h>
#include <lib/log.h>
#include <stdint.h>

static uint64_t g_boot_tsc;
static uint32_t g_tsc_ticks_per_us;

static uint64_t g_realtime_base_ns;
static uint64_t g_realtime_mono_ref_ns;

void time_init(void) {
    g_boot_tsc = __rdtsc();
    g_tsc_ticks_per_us = CPU_LOCAL_READ(tsc_ticks_per_us);

    g_realtime_base_ns = g_bootloader_info.boot_timestamp * 1000000000ULL;
    g_realtime_mono_ref_ns = 0;

    LOG_OKAY("time: monotonic clock initialised (boot_tsc=%llu, %u ticks/us)\n", (unsigned long long) g_boot_tsc, g_tsc_ticks_per_us);
    LOG_OKAY("time: realtime seeded from boot_timestamp=%llu s\n", (unsigned long long) g_bootloader_info.boot_timestamp);
}

uint64_t time_monotonic_ns(void) {
    uint64_t tsc_now = __rdtsc();
    uint64_t elapsed_ticks = tsc_now - g_boot_tsc;
    return (elapsed_ticks / g_tsc_ticks_per_us) * 1000ULL;
}

uint64_t time_realtime_ns(void) {
    uint64_t mono = time_monotonic_ns();
    return g_realtime_base_ns + (mono - g_realtime_mono_ref_ns);
}

void time_sync_realtime(uint64_t unix_ns) {
    g_realtime_mono_ref_ns = time_monotonic_ns();
    g_realtime_base_ns = unix_ns;
    LOG_OKAY("time: realtime synced to %llu ns\n", (unsigned long long) unix_ns);
}
