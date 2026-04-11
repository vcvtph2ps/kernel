#include <common/arch.h>
#include <common/irql.h>
#include <lib/spinlock.h>

static inline bool spinlock_try_lock(volatile spinlock_t* lock) {
    return !ATOMIC_XCHG(&lock->lock, 1, ATOMIC_ACQUIRE);
}

static inline void spinlock_unlock_raw(volatile spinlock_t* lock) {
    ATOMIC_STORE(&lock->lock, 0, ATOMIC_RELEASE);
}

static inline void spinlock_lock_raw(volatile spinlock_t* lock) {
    while(true) {
        if(spinlock_try_lock(lock)) return;
        while(ATOMIC_LOAD(&lock->lock, ATOMIC_RELAXED)) { arch_relax(); }
    }
}

irql_t spinlock_lock(spinlock_t* lock) {
    irql_t prev = irql_raise(IRQL_DISPATCH);
    spinlock_lock_raw(lock);
    return prev;
}

irql_t spinlock_lock_raise(spinlock_t* lock, irql_t irql) {
    irql_t prev = irql_raise(irql);
    spinlock_lock_raw(lock);
    return prev;
}

void spinlock_unlock(spinlock_t* lock, irql_t prev_irql) {
    spinlock_unlock_raw(lock);
    (void) irql_lower(prev_irql);
}
