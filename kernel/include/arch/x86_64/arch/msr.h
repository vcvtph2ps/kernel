#pragma once
#include <stdint.h>

// NOLINTBEGIN
// MSR numbers
#define IA32_APIC_BASE_MSR 0x1B
#define IA32_X2APIC_BASE_MSR 0x800
#define IA32_PAT_MSR 0x277

// APIC Base MSR flags
#define APIC_BASE_BSP (1 << 8)
#define APIC_BASE_ENABLE (1 << 11)
#define APIC_BASE_X2APIC (1 << 10)

// MSRs for segment bases
#define IA32_FS_BASE_MSR 0xC0000100
#define IA32_GS_BASE_MSR 0xC0000101
#define IA32_KERNEL_GS_BASE_MSR 0xC0000102

// extended feature enable register
#define IA32_EFER 0xC0000080

// syscall stuff
#define IA32_STAR 0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_CSTAR 0xC0000083
#define IA32_SFMASK 0xC0000084

// fred
#define IA32_FRED_CONFIG 0x000001D4
#define IA32_FRED_RSP0 0x000001CC
#define IA32_FRED_RSP1 0x000001CD
#define IA32_FRED_RSP2 0x000001CE
#define IA32_FRED_RSP3 0x000001CF
#define IA32_FRED_STACK_LEVELS 0x000001D0
// NOLINTEND

/**
 * @brief Reads the value of a Model-Specific Register
 * @param msr The number of the MSR to read.
 * @return The value of the specified MSR.
 */
[[nodiscard]] uint64_t arch_msr_read_msr(uint32_t msr);

/**
 * @brief Writes a value to a Model-Specific Register
 * @param msr The number of the MSR to write to.
 * @param value The value to write to the specified MSR.
 */
void arch_msr_write_msr(uint32_t msr, uint64_t value);
