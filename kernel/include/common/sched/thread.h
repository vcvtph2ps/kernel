#pragma once
#include <common/interrupts/dw.h>
#include <lib/list.h>
#include <memory/memory.h>

typedef enum thread_state thread_state_t;
typedef struct thread thread_t;
typedef struct process process_t; // NOLINT
typedef struct scheduler scheduler_t; // NOLINT

enum thread_state {
    THREAD_STATE_READY,
    THREAD_STATE_RUNNING,
    THREAD_STATE_DEAD
};

struct thread {
    uint32_t tid;
    thread_state_t state;

    scheduler_t* sched;
    process_t* process;

    list_node_t list_node_sched;
    list_node_t list_node_proc;

    bool in_interrupt_handler;

    struct {
        bool in_process;
        virt_addr_t address;
        dw_item_t dw_item;
    } vm_fault;
};
