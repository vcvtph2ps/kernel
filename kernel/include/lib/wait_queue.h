#pragma once
#include <common/sched/thread.h>
#include <list.h>
#include <spinlock.h>

typedef struct {
    spinlock_no_dw_t lock;
    list_t threads;
} wait_queue_t;

#define WAIT_QUEUE_INIT ((wait_queue_t) { .threads = LIST_INIT, .lock = SPINLOCK_NO_DW_INIT })

/**
 * @brief Put the current thread into the end of the wait queue and yield
 * @param wq A pointer to the wait queue to join a thread from.
 */
void wait_queue_join(wait_queue_t* wq);

/**
 * @brief Wakes up the first thread in the wait queue. The woken thread is removed from the wait queue and put in the ready state.
 * @param wq A pointer to the wait queue to wake a thread from.
 * @return true if a thread was woken, false if the wait queue was empty
 */
bool wait_queue_wake_one(wait_queue_t* wq);

/*
 * @brief Wakes up all threads from the wait queue. The woken threads will be removed from the wait queue and put in the ready state
 * @param wq A pointer to the wait queue to wake threads from
 * @return Number of threads that were awoken
 */
int wait_queue_wake_all(wait_queue_t* wq);
