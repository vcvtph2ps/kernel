#include <common/sched/sched.h>
#include <common/sched/thread.h>
#include <common/userspace/process.h>
#include <fs/fd_store.h>
#include <lib/helpers.h>
#include <log.h>
#include <memory/heap.h>
#include <stdint.h>

ATOMIC uint32_t g_next_task_id = 1;

list_t g_process_list = LIST_INIT;
spinlock_no_dw_t g_process_list_lock = SPINLOCK_NO_DW_INIT;

uint32_t process_allocate_id() {
    return ATOMIC_LOAD_ADD(&g_next_task_id, 1, ATOMIC_RELAXED);
}

process_t* process_create_empty() {
    process_t* process = heap_alloc(sizeof(process_t));
    process->pid = process_allocate_id();
    process->address_space = nullptr;
    process->thread_list_lock = SPINLOCK_NO_DW_INIT;
    process->threads = LIST_INIT;
    process->fd_store = fd_store_create();

    vfs_result_t res;
    res = fd_store_open_fixed(process->fd_store, &VFS_MAKE_ABS_PATH("/hello.txt"), 0);
    assert(res == VFS_RESULT_OK);
    res = fd_store_open_fixed(process->fd_store, &VFS_MAKE_ABS_PATH("/hello.txt"), 1);
    assert(res == VFS_RESULT_OK);
    res = fd_store_open_fixed(process->fd_store, &VFS_MAKE_ABS_PATH("/hello.txt"), 2);
    assert(res == VFS_RESULT_OK);

    res = vfs_root(&process->cwd);
    assert(res == VFS_RESULT_OK);

    spinlock_nodw_lock(&g_process_list_lock);
    list_push_back(&g_process_list, &process->process_list_node);
    spinlock_nodw_unlock(&g_process_list_lock);
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

process_t* process_get_by_pid(uint32_t pid) {
    spinlock_nodw_lock(&g_process_list_lock);
    list_node_t* node = g_process_list.head;
    while(node) {
        process_t* process = CONTAINER_OF(node, process_t, process_list_node);
        if(process->pid == pid) {
            spinlock_nodw_unlock(&g_process_list_lock);
            return process;
        }
        node = node->next;
    }
    spinlock_nodw_unlock(&g_process_list_lock);
    return nullptr;
}
