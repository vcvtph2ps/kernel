#pragma once
#include <common/arch.h>
#include <common/irql.h>
#include <lib/helpers.h>
#include <stdint.h>

typedef struct {
    ATOMIC uint32_t lock;
} spinlock_t;

#define SPINLOCK_INIT ((spinlock_t) { 0, 0 })

/**
 * @brief Acquires a spinlock and raises IRQL to DISPATCH_LEVEL.
 *
 * @param lock Pointer to the spinlock to acquire.
 * @return The previous IRQL before it was raised.
 */
[[nodiscard]] irql_t spinlock_lock(spinlock_t* lock);

/**
 * @brief Acquires a spinlock and raises IRQL to a specified level.
 *
 * @param lock Pointer to the spinlock to acquire.
 * @param irql The IRQL level to raise to.
 * @return The previous IRQL before it was raised.
 */
[[nodiscard]] irql_t spinlock_lock_raise(spinlock_t* lock, irql_t irql);

/**
 * @brief Releases a spinlock and restores the previous IRQL.
 *
 * @param lock Pointer to the spinlock to release.
 * @param prev_irql The IRQL value to restore.
 */
void spinlock_unlock(spinlock_t* lock, irql_t prev_irql);
