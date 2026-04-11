#pragma once
#include <arch/internal/cr.h>
#include <limine.h>
#include <stddef.h>

/**
 * @brief Initializes the architecture-specific code for the bootstrapping processor (BSP). Should only be called once during early initialization before any other cores are started.
 */
void arch_init_bsp(void);

/**
 * @brief Initializes the architecture-specific code for an application processor (AP). Should only be called once per AP during early initialization of that AP before it starts running any non-arch-specific code.
 * @param core_id The core id of the AP being initialized. This is passed in by the bootloader when starting the AP and should be used by the AP to identify itself and to determine which core it is running on.
 */
void arch_init_ap(uint32_t core_id);

/**
 * @brief Gets the name of the architecture that the kernel is running on. This is used for informational purposes
 * @return A string literal that identifies the architecture in a human-readable way (e.g. "x86_64", "riscv64", etc.).
 */
const char* arch_get_name(void);

/**
 * @brief Waits for the next interrupt. This should only be called when the kernel has nothing else to do
 */
void arch_wait_for_interrupt(void);

/**
 * @brief Issues a memory barrier to ensure that all memory operations issued before the barrier are completed before any memory operations issued after the barrier.
 */
void arch_memory_barrier(void);

/**
 * @brief Issues a relax instruction that is a hint to the processor that we are in a tight spinloop and should relax for that case
 */
void arch_relax(void);

/**
 * @brief Panics the kernel with the specified message. This should only be called in unrecoverable error conditions where the kernel cannot continue running.
 */
[[noreturn]] void arch_panic(const char* fmt, ...);

/**
 * @brief Just kills the current core by looping forever
 */
[[noreturn]] void arch_die();

/**
 * @return true if the current processor is the bootstrap processor, false if the processor is an application processor
 */
bool arch_is_bsp(void);

/**
 * @return The unique core id of the current processor.
 */
uint32_t arch_get_core_id(void);

/**
 * @return The total number of cores in the system.
 */
uint32_t arch_get_core_count(void);

/**
 * @brief dumps `c` into some unknown output that is useful for debugging.
 */
void arch_debug_putc(char c);
