#include <arch/msr.h>

void x86_64_handle_syscall();

void arch_syscall_init() {
    uint64_t efer = arch_msr_read_msr(IA32_EFER);
    efer |= (1 << 0);
    arch_msr_write_msr(IA32_EFER, efer);

    uint64_t star = ((uint64_t) (0x18 | 3) << 48) | ((uint64_t) 0x08 << 32);
    arch_msr_write_msr(IA32_STAR, star);

    arch_msr_write_msr(IA32_LSTAR, (uint64_t) x86_64_handle_syscall);
    arch_msr_write_msr(IA32_SFMASK, ~0x2);
}
