#include <common/arch.h>

#include "log.h"

[[noreturn]] void arch_panic(const char* fmt, ...) {
    __asm__ volatile("cli" ::: "memory");
    __asm__ volatile("mfence" ::: "memory");
    __asm__ volatile("lfence" ::: "memory");

    // ipi_broadcast_die();
    int apic_id = arch_get_core_id();

    log_print_nolock(LOG_LEVEL_FAIL, "Kernel panic: ");
    va_list args;
    va_start(args, fmt);
    nl_vprintf(fmt, args);
    va_end(args);
    log_print_nolock(LOG_LEVEL_FAIL, "\nOn core: %d\n\n", apic_id);

    log_print_nolock(LOG_LEVEL_FAIL, "mrrp mrrp meow meow mrrp. oops\n");
    log_print_nolock(LOG_LEVEL_FAIL, "                   _ |\\_\n");
    log_print_nolock(LOG_LEVEL_FAIL, "                   \\` ..\\\n");
    log_print_nolock(LOG_LEVEL_FAIL, "              __,.-\" =__Y=\n");
    log_print_nolock(LOG_LEVEL_FAIL, "            .\"        )\n");
    log_print_nolock(LOG_LEVEL_FAIL, "      _    /   ,    \\/\\_\n");
    log_print_nolock(LOG_LEVEL_FAIL, "     ((____|    )_-\\ \\_-\\`\n");
    log_print_nolock(LOG_LEVEL_FAIL, "     `-----'`-----` `--`\n");

    while(1) {
        __builtin_ia32_pause();
        asm volatile("hlt");
    }
    __builtin_unreachable();
}
