#include <arch/cpu_local.h>
#include <common/sched/sched.h>
#include <common/sched/thread.h>
#include <common/time.h>
#include <common/userspace/structs.h>
#include <common/userspace/syscall.h>
#include <common/userspace/syscalls/sys_futex.h>
#include <hashmap.h>
#include <memory/ptm.h>
#include <memory/vm.h>
#include <spinlock.h>
#include <wait_queue.h>

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

#define FUTEX_BUCKETS 256

typedef struct {
    uintptr_t phys_addr;
    process_t* proc;
    wait_queue_t wq;
    hashmap_node_t map_node;
} futex_entry_t;

static hashmap_t g_futex_map;

static spinlock_no_int_t g_futex_lock = SPINLOCK_NO_INT_INIT;

static list_t g_timeout_list = LIST_INIT;

static size_t futex_hash(process_t* proc, uintptr_t uaddr) {
    hasher_t hasher = hasher_new();
    hasher_hash(&hasher, proc->pid);
    hasher_hash(&hasher, uaddr);
    return hasher_finalize(hasher);
}

static futex_entry_t* futex_find(process_t* proc, uintptr_t uaddr) {
    uintptr_t target_phys;
    if(!ptm_physical(proc->address_space, uaddr, &target_phys)) {
        return nullptr;
    }
    list_t* bucket = hashmap_bucket(&g_futex_map, futex_hash(proc, target_phys));
    list_node_t* node = bucket->head;

    while(node != nullptr) {
        futex_entry_t* entry = CONTAINER_OF(node, futex_entry_t, map_node.list_node);
        if(entry->proc == proc && entry->phys_addr == target_phys) return entry;
        node = node->next;
    }
    return nullptr;
}

static futex_entry_t* futex_find_or_create(process_t* proc, uintptr_t uaddr) {
    futex_entry_t* entry = futex_find(proc, uaddr);
    if(entry != nullptr) return entry;

    entry = (futex_entry_t*) heap_alloc(sizeof(futex_entry_t));
    if(entry == nullptr) return nullptr;
    uintptr_t target_phys;
    if(!ptm_physical(proc->address_space, uaddr, &target_phys)) {
        return nullptr;
    }

    entry->phys_addr = target_phys;
    entry->proc = proc;
    entry->wq = WAIT_QUEUE_INIT;

    hashmap_insert(&g_futex_map, futex_hash(proc, target_phys), &entry->map_node);
    return entry;
}

void sys_futex_init() {
    if(!hashmap_init(&g_futex_map, FUTEX_BUCKETS)) {
        arch_panic("Failed to initialize futex hashmap\n");
    }
}

void sys_futex_check_timeouts(void) {
    uint64_t now = time_monotonic_ns();

    uint64_t irq_state = spinlock_noint_lock(&g_futex_lock);

    list_node_t* node = g_timeout_list.head;
    while(node != nullptr) {
        list_node_t* next = node->next;
        thread_t* thread = CONTAINER_OF(node, thread_t, list_node_timeout);

        if(now >= thread->timeout_deadline_ns) {
            list_node_delete(&g_timeout_list, &thread->list_node_timeout);
            thread->timeout_in_list = false;
            thread->timeout_deadline_ns = 0;

            futex_entry_t* entry = (futex_entry_t*) thread->timeout_ctx;
            list_node_delete(&entry->wq.threads, &thread->list_node_wait_queue);

            thread->timed_out = true;
            sched_thread_schedule(thread);
        }

        node = next;
    }

    spinlock_noint_unlock(&g_futex_lock, irq_state);
}

static syscall_ret_t futex_wait(uintptr_t uaddr, int32_t val, structs_timespec_t* timeout_struct) {
    uint64_t deadline_ns = 0;
    if(timeout_struct != nullptr) {
        if(timeout_struct->tv_sec < 0 || timeout_struct->tv_nsec < 0 || timeout_struct->tv_nsec >= 1000000000L) {
            return SYSCALL_RET_ERROR(SYSCALL_ERROR_INVAL);
        }
        uint64_t timeout_ns = (uint64_t) timeout_struct->tv_sec * 1000000000ULL + (uint64_t) timeout_struct->tv_nsec;
        deadline_ns = time_monotonic_ns() + timeout_ns;
    }

    uint64_t irq_state = spinlock_noint_lock(&g_futex_lock);

    int32_t current_val = 0;
    vm_copy_from(&current_val, CPU_LOCAL_GET_CURRENT_THREAD()->common.process->address_space, uaddr, sizeof(int32_t));

    if(current_val != val) {
        spinlock_noint_unlock(&g_futex_lock, irq_state);
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_AGAIN);
    }

    futex_entry_t* entry = futex_find_or_create(CPU_LOCAL_GET_CURRENT_THREAD()->common.process, uaddr);
    if(entry == nullptr) {
        spinlock_noint_unlock(&g_futex_lock, irq_state);
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_NOMEM);
    }

    thread_t* current = &CPU_LOCAL_GET_CURRENT_THREAD()->common;

    current->timed_out = false;
    current->timeout_in_list = false;
    current->timeout_deadline_ns = 0;
    current->timeout_ctx = nullptr;

    list_push_back(&entry->wq.threads, &current->list_node_wait_queue);

    if(deadline_ns != 0) {
        current->timeout_deadline_ns = deadline_ns;
        current->timeout_ctx = entry;
        current->timeout_in_list = true;
        list_push_back(&g_timeout_list, &current->list_node_timeout);
    }

    spinlock_noint_unlock(&g_futex_lock, irq_state);

    sched_yield(THREAD_STATE_BLOCKED);

    if(deadline_ns != 0) {
        irq_state = spinlock_noint_lock(&g_futex_lock);
        if(current->timeout_in_list) {
            list_node_delete(&g_timeout_list, &current->list_node_timeout);
            current->timeout_in_list = false;
        }
        current->timeout_deadline_ns = 0;
        current->timeout_ctx = nullptr;
        spinlock_noint_unlock(&g_futex_lock, irq_state);
    }

    if(current->timed_out) {
        current->timed_out = false;
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_TIMEDOUT);
    }

    return SYSCALL_RET_ERROR(0);
}

static syscall_ret_t futex_wake(uintptr_t uaddr, int32_t val, structs_timespec_t* timeout_struct) {
    (void) timeout_struct;

    int32_t woken = 0;

    uint64_t irq_state = spinlock_noint_lock(&g_futex_lock);

    futex_entry_t* entry = futex_find(CPU_LOCAL_GET_CURRENT_THREAD()->common.process, uaddr);
    if(entry != nullptr) {
        while(val < 0 || woken < val) {
            list_node_t* node = list_pop(&entry->wq.threads);
            if(!node) {
                break;
            }

            thread_t* thread = CONTAINER_OF(node, thread_t, list_node_wait_queue);

            if(thread->timeout_in_list) {
                list_node_delete(&g_timeout_list, &thread->list_node_timeout);
                thread->timeout_in_list = false;
                thread->timeout_deadline_ns = 0;
                thread->timeout_ctx = nullptr;
            }

            sched_thread_schedule(thread);
            woken++;
        }
    }

    spinlock_noint_unlock(&g_futex_lock, irq_state);
    return SYSCALL_RET_ERROR(woken);
}

syscall_ret_t syscall_sys_futex(syscall_args_t args) {
    uintptr_t uaddr = args.arg1;
    uint64_t op = args.arg2;
    int32_t val = (int32_t) args.arg3;
    uintptr_t timeout_ptr = args.arg4;

    process_t* proc = CPU_LOCAL_GET_CURRENT_THREAD()->common.process;
    if(!vm_validate_buffer(proc->address_space, uaddr, sizeof(int32_t))) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT);
    }


    structs_timespec_t timeout_buf = {};
    structs_timespec_t* timeout = nullptr;
    if(timeout_ptr != 0) {
        if(!vm_validate_buffer(proc->address_space, timeout_ptr, sizeof(structs_timespec_t))) {
            return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT);
        }
        vm_copy_from(&timeout_buf, proc->address_space, timeout_ptr, sizeof(structs_timespec_t));
        timeout = &timeout_buf;
    }

    if(op == FUTEX_WAIT) {
        LOG_STRC("syscall_sys_futex: op=FUTEX_WAIT pid=%lu uaddr=0x%lx val=%d timeout=%s\n", proc->pid, uaddr, val, timeout ? "yes" : "none");
        return futex_wait(uaddr, val, timeout);
    } else if(op == FUTEX_WAKE) {
        LOG_STRC("syscall_sys_futex: op=FUTEX_WAKE pid=%lu uaddr=0x%lx val=%d\n", proc->pid, uaddr, val);
        return futex_wake(uaddr, val, timeout);
    } else {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_INVAL);
    }
}
