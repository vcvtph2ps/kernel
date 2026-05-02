#include <arch/cpu_local.h>
#include <common/sched/sched.h>
#include <common/sched/thread.h>
#include <lib/wait_queue.h>
#include <list.h>
#include <spinlock.h>

void wait_queue_join(wait_queue_t* wq) {
    thread_t* current = &CPU_LOCAL_GET_CURRENT_THREAD()->common;
    spinlock_nodw_lock(&wq->lock);
    list_push_back(&wq->threads, &current->list_node_wait_queue);
    spinlock_nodw_unlock(&wq->lock);
    sched_yield(THREAD_STATE_BLOCKED);
}

bool wait_queue_wake_one(wait_queue_t* wq) {
    spinlock_nodw_lock(&wq->lock);
    list_node_t* node = list_pop(&wq->threads);
    spinlock_nodw_unlock(&wq->lock);
    if(node) {
        thread_t* thread = CONTAINER_OF(node, thread_t, list_node_wait_queue);
        sched_thread_schedule(thread);
        return true;
    }
    return false;
}

int wait_queue_wake_all(wait_queue_t* wq) {
    int woke_count = 0;
    while(true) {
        spinlock_nodw_lock(&wq->lock);
        list_node_t* node = list_pop(&wq->threads);
        spinlock_nodw_unlock(&wq->lock);

        if(node == nullptr) break;
        thread_t* thread = CONTAINER_OF(node, thread_t, list_node_wait_queue);
        sched_thread_schedule(thread);
        woke_count++;
    }
    return woke_count;
}
