#pragma once
#include <memory/memory.h>
#include <stdint.h>

/**
 * @brief Initializes the IOAPIC on the BSP. This should be called once during early initialization
 */
void arch_ioapic_init_bsp();

/**
 * @brief Map GSI to interrupt vector
 * @param gsi Global system interrupt (IRQ)
 * @param lapic_id Local apic ID
 * @param low_polarity true = high active, false = low active
 * @param trigger_mode true = edge sensitive, false = level sensitive
 * @param vector Interrupt vector
 */
void arch_ioapic_map_gsi(uint8_t gsi, uint8_t lapic_id, bool polarity, bool trigger_mode, uint8_t vector);

/**
 * @brief Map legacy IRQ to interrupt vector
 * @param irq Legacy IRQ
 * @param lapic_id Local apic ID
 * @param fallback_low_polarity true = high active, false = low active
 * @param fallback_trigger_mode true = edge sensitive, false = level sensitive
 * @param vector Interrupt vector
 */
void arch_ioapic_map_legacy_irq(uint8_t irq, uint8_t lapic_id, bool fallback_polarity, bool fallback_trigger_mode, uint8_t vector);
