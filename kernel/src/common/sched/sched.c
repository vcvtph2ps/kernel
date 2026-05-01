#include <arch/cpu_local.h>
#include <assert.h>
#include <common/arch.h>
#include <common/interrupts/interrupt.h>
#include <common/sched/sched.h>
#include <common/sched/thread.h>

void sched_preempt_disable() {
    assert(CPU_LOCAL_READ(preempt.counter) < UINT32_MAX);
    CPU_LOCAL_INC(preempt.counter);
}

void sched_preempt_enable() {
    size_t count = CPU_LOCAL_READ(preempt.counter);
    assert(count > 0);
    bool yield = count == 1 && CPU_LOCAL_EXCHANGE(preempt.yield_pending, false);
    CPU_LOCAL_DEC(preempt.counter);
    if(yield) sched_yield(THREAD_STATE_READY);
}

void idle_thread_entry() {
    while(1) { arch_wait_for_interrupt(); }
}

thread_t* sched_next_thread(scheduler_t* sched) {
    spinlock_lock(&sched->lock);
    list_node_t* node = list_pop(&sched->thread_queue);
    thread_t* next = node ? CONTAINER_OF(node, thread_t, list_node_sched) : nullptr;
    spinlock_unlock(&sched->lock);
    return next;
}

void sched_thread_schedule(thread_t* thread) {
    thread->state = THREAD_STATE_READY;
    spinlock_lock(&thread->sched->lock);
    list_push_back(&thread->sched->thread_queue, &thread->list_node_sched);
    spinlock_unlock(&thread->sched->lock);
}
void sched_arch_init_bsp();

void sched_init_bsp() {
    scheduler_t* sched = &CPU_LOCAL_GET_SELF()->sched;
    sched->thread_queue = LIST_INIT;
    sched->lock = SPINLOCK_INIT;
    sched->idle_thread = sched_arch_create_thread_kernel((virt_addr_t) idle_thread_entry);
    sched_arch_init_bsp();
}


void sched_yield(thread_state_t yield_state) {
    uint64_t previous_state = arch_disable_interupts();

    assert(yield_state != THREAD_STATE_RUNNING);
    assert(CPU_LOCAL_READ(preempt.counter) == 0);
    assert(CPU_LOCAL_READ(defered_work.counter) == 0);

    thread_t* current = sched_arch_thread_current();

    thread_t* next = sched_next_thread(&CPU_LOCAL_GET_SELF()->sched);
    if(next == nullptr && current != current->sched->idle_thread) next = current->sched->idle_thread;
    if(next != nullptr) {
        assert(current != next);
        current->state = yield_state;
        sched_arch_context_switch(current, next);
    } else {
        assert(current->state == yield_state);
    }

    sched_arch_reset_preempt_timer();
    arch_restore_interupts(previous_state);
}

[[noreturn]] void sched_arch_handoff() {
    thread_t* bsp_thread = sched_arch_create_thread_kernel(0);
    bsp_thread->state = THREAD_STATE_DEAD;

    scheduler_t* sched = &CPU_LOCAL_READ(self)->sched;
    thread_t* idle_thread = sched->idle_thread;

    CPU_LOCAL_WRITE(preempt.threaded, true);
    (void) arch_enable_interupts();
    sched_arch_context_switch(bsp_thread, idle_thread);
    while(1);
}
