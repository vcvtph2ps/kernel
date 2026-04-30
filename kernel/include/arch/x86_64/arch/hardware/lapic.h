#pragma once
#include <memory/memory.h>
#include <stdint.h>

/**
 * @brief Initializes the LAPIC on the BSP. This should be called once during the early init of the kernel on the BSP.
 */
void arch_lapic_init_bsp();

/**
 * @brief Initializes the LAPIC on an AP. This should be called once before cpu local is setup on the AP.
 */
void arch_lapic_init_ap_early();

/**
 * @brief Finishes initializing the LAPIC on an AP. This should be called once during the init of the kernel on each AP.
 */
void arch_lapic_init_ap();

/**
 * @brief Get the APIC ID of the current processor
 * @return The APIC ID of the current processor
 */
uint32_t arch_lapic_get_id();

/**
 * @brief Checks if the current processor is the bootstrap processor
 * @returns True if the current process is the bootstrap processor
 */
bool arch_lapic_is_bsp();

/**
 * @brief Sends end of interrupt to the LAPIC
 */
void arch_lapic_eoi();

/**
 * @brief Configures the LAPIC timer to fire a one-shot interrupt after the specified number of microseconds.
 * @param microseconds The number of microseconds to wait before firing the interrupt.
 */
void arch_lapic_timer_oneshot_us(uint32_t microseconds);

/**
 * @brief Configures the LAPIC timer to fire a one-shot interrupt after the specified number of milliseconds.
 * @param milliseconds The number of milliseconds to wait before firing the interrupt.
 */
void arch_lapic_timer_oneshot_ms(uint32_t milliseconds);

/**
 * @brief Stops any currently active LAPIC timer countdown, if there is one.
 */
void arch_lapic_timer_stop();

/**
 * @brief Sends an interprocessor interrupt with the specified vector to the processor with the specified APIC ID.
 * @param apic_id The APIC ID of the target processor to send the IPI
 * @param vector The interrupt vector to send in the IPI.
 */
void arch_lapic_send_ipi(uint32_t apic_id, uint8_t vector);

/**
 * @brief Broadcasts an interprocessor interrupt with the specified vector to all other processors in the system.
 * @param vector The interrupt vector to send in the IPI.
 */
void arch_lapic_broadcast_ipi(uint8_t vector);
