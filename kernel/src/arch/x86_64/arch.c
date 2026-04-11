#include <arch/cpu_local.h>
#include <arch/internal/cpuid.h>
#include <arch/internal/cr.h>
#include <common/boot/bootloader.h>
#include <common/cpu_local.h>
#include <common/io.h>
#include <memory/memory.h>

const char* arch_get_name(void) {
    return "x86_64";
}


void arch_wait_for_interrupt(void) {
    __asm__ volatile("hlt");
}

void arch_memory_barrier(void) {
    __asm__ volatile("mfence" ::: "memory");
}

void arch_relax() {
    __asm__ volatile("pause" ::: "memory");
}

[[noreturn]] void arch_die(void) {
    for(;;) {
        __asm__ volatile("cli");
        __asm__ volatile("pause");
        __asm__ volatile("hlt");
    }
}

void arch_debug_putc(char c) {
    arch_io_port_write_u8(0xe9, (uint8_t) c);
}


uint32_t arch_get_core_id() {
    return CPU_LOCAL_READ(core_id);
}

bool arch_is_bsp() {
    return CPU_LOCAL_READ(core_id) == 0;
}

uint32_t arch_get_core_count() {
    return g_bootloader_info.cpu_count;
}
