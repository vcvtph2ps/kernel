#include <arch/hardware/lapic.h>
#include <arch/hardware/pit.h>
#include <arch/internal/cpuid.h>
#include <arch/msr.h>
#include <common/io.h>
#include <log.h>
#include <memory/memory.h>
#include <memory/vm.h>
#include <stdint.h>

#include "common/init.h"

#define LAPIC_TIMER_INIT_COUNT 0x380
#define LAPIC_TIMER_CUR_COUNT 0x390
#define LAPIC_TIMER_DIV 0x3E0
#define LAPIC_LVT_TIMER 0x320
#define LAPIC_MASK_MASKED 0x10000

uint32_t lapic_read(uint32_t reg);
uint32_t lapic_write(uint32_t reg, uint32_t value);

static uint32_t g_lapic_timer_ticks_per_us = 0;

void lapic_timer_init(uint32_t core_id) {
    lapic_write(LAPIC_TIMER_DIV, 0x3);

    if(!INIT_CORE_IS_BSP(core_id)) {
        return;
    }

    lapic_write(LAPIC_TIMER_INIT_COUNT, 0xFFFFFFFF);

    arch_pit_sleep_us(10000);

    uint32_t elapsed = 0xFFFFFFFF - lapic_read(LAPIC_TIMER_CUR_COUNT);
    g_lapic_timer_ticks_per_us = elapsed / 10000ull;

    LOG_OKAY("apic timer calibrated: %d ticks/us\n", g_lapic_timer_ticks_per_us);
}

void arch_lapic_timer_oneshot_us(uint32_t microseconds) {
    lapic_write(LAPIC_TIMER_DIV, 0x3);
    lapic_write(LAPIC_LVT_TIMER, 0x20);
    uint32_t ticks = (uint32_t) (microseconds * g_lapic_timer_ticks_per_us);
    lapic_write(LAPIC_TIMER_INIT_COUNT, ticks);
}

void arch_lapic_timer_oneshot_ms(uint32_t milliseconds) {
    arch_lapic_timer_oneshot_us(milliseconds * 1000);
}

void arch_lapic_timer_stop() {
    lapic_write(LAPIC_LVT_TIMER, LAPIC_MASK_MASKED);
}
