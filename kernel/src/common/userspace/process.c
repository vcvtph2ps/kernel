#include <common/sched/sched.h>
#include <common/sched/thread.h>
#include <common/userspace/process.h>
#include <lib/helpers.h>
#include <log.h>
#include <memory/heap.h>
#include <stdint.h>

ATOMIC uint32_t g_next_task_id = 1;

uint32_t process_allocate_id() {
    return ATOMIC_LOAD_ADD(&g_next_task_id, 1, ATOMIC_RELAXED);
}

process_t* process_create_empty() {
    process_t* process = heap_alloc(sizeof(process_t));
    process->pid = process_allocate_id();
    process->address_space = nullptr;
    process->thread_list_lock = SPINLOCK_NO_DW_INIT;
    process->threads = LIST_INIT;
    return process;
}

process_t* process_create(vm_address_space_t* address_space, virt_addr_t entry, virt_addr_t user_stack) {
    process_t* process = process_create_empty();
    process->address_space = address_space;
    thread_t* thread = sched_arch_create_thread_user(process, user_stack, entry, true);
    list_push_back(&process->threads, &thread->list_node_proc);
    LOG_INFO("Created process with pid %u\n", process->pid);
    return process;
}
