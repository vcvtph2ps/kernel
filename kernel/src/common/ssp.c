#include <common/arch.h>
#include <log.h>
#include <stdint.h>

uint64_t __stack_chk_guard = 0xdeadbeefcafebabe; // NOLINT

[[noreturn]] void __stack_chk_fail(void) { // NOLINT
    log_print_nolock(LOG_LEVEL_FAIL, "Stack smashing detected on CPU %u\n", arch_get_core_id());
    arch_die();
}
