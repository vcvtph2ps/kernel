#pragma once
#include <stdint.h>

/**
 * @brief Allocates an interrupt vector and returns its number.
 * @return The allocated vector number, or 0 if no vectors are available.
 */
uint8_t vector_alloc_interrupt();

/**
 * @brief Allocates a specific interrupt vector.
 * @param vector The vector number to allocate.
 * @return The allocated vector number. or 0 if the vector is already allocated or out of range.
 */
uint8_t vector_alloc_specific_interrupt(uint8_t vector);

/**
 * @brief Frees an interrupt vector, making it available for future allocations.
 */
void vector_free_interrupt(uint8_t vector); // NOLINT
