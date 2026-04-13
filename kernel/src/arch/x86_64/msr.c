#include <arch/cpu_local.h>
#include <arch/msr.h>
#include <assert.h>
#include <limine.h>
#include <memory/memory.h>

[[nodiscard]] uint64_t arch_msr_read_msr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t) hi << 32) | lo;
}

void arch_msr_write_msr(uint32_t msr, uint64_t value) {
    uint32_t lo = (uint32_t) (value & 0xFFFFFFFF);
    uint32_t hi = (uint32_t) (value >> 32);
    __asm__ volatile("wrmsr" : : "c"(msr), "a"(lo), "d"(hi));
}
