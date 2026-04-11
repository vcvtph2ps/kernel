#pragma once
#include <stdint.h>

typedef enum irql : uint8_t {
    IRQL_PASSIVE = 0,
    IRQL_DISPATCH = 2,
    IRQL_HIGH = 15
} irql_t;

/**
 * @brief Raises the IRQL to the new level, which must be greater than or equal to the current IRQL level.
 * @param new_level The IRQL level to raise to.
 * @return the previous IRQL level
 */
[[nodiscard]] irql_t irql_raise(irql_t new_level);

/**
 * @brief Lowers the IRQL to the new level, which must be less than or equal to the current IRQL level.
 * @param new_level The IRQL level to lower to.
 * @return the previous IRQL level
 */
[[nodiscard]] irql_t irql_lower(irql_t new_level);

/**
 * @brief Gets the current IRQL level.
 * @return the current IRQL level
 */
[[nodiscard]] irql_t irql_get();
