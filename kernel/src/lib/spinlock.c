#include <common/arch.h>
#include <common/interrupts/dw.h>
#include <common/sched/sched.h>
#include <lib/spinlock.h>

static inline bool spinlock_try_lock(ATOMIC_PARAM uint32_t* lock) {
    return !ATOMIC_XCHG(lock, 1, ATOMIC_ACQUIRE);
}

static inline void spinlock_unlock_raw(ATOMIC_PARAM uint32_t* lock) {
    ATOMIC_STORE(lock, 0, ATOMIC_RELEASE);
}

static inline void spinlock_lock_raw(ATOMIC_PARAM uint32_t* lock) {
    while(true) {
        if(spinlock_try_lock(lock)) return;
        while(ATOMIC_LOAD(lock, ATOMIC_RELAXED)) {
            arch_relax();
        }
    }
}

void spinlock_lock(spinlock_t* lock) {
    sched_preempt_disable();
    spinlock_lock_raw(&lock->lock);
}

void spinlock_unlock(spinlock_t* lock) {
    spinlock_unlock_raw(&lock->lock);
    sched_preempt_enable();
}

void spinlock_nodw_lock(spinlock_no_dw_t* lock) {
    sched_preempt_disable();
    dw_status_disable();
    spinlock_lock_raw(&lock->lock);
}

void spinlock_nodw_unlock(spinlock_no_dw_t* lock) {
    spinlock_unlock_raw(&lock->lock);
    dw_status_enable();
    sched_preempt_enable();
}


[[nodiscard]] uint64_t spinlock_noint_lock(spinlock_no_int_t* lock) {
    uint64_t state = arch_disable_interupts();
    spinlock_lock_raw(&lock->lock);
    return state;
}

void spinlock_noint_unlock(spinlock_no_int_t* lock, uint64_t interrupt_state) {
    spinlock_unlock_raw(&lock->lock);
    arch_restore_interupts(interrupt_state);
}
