#pragma once
#include <fs/fd_store.h>
#include <fs/vfs.h>
#include <lib/list.h>
#include <memory/vm.h>
#include <spinlock.h>

typedef struct process process_t;

struct process {
    uint32_t pid;
    vm_address_space_t* address_space;

    spinlock_no_dw_t thread_list_lock;
    list_t threads;
    fd_store_t* fd_store;
    vfs_node_t* cwd;
};

/**
 * @brief Allocates a unique process ID (PID)
 * @return A unique process ID that has not been used
 */
uint32_t process_allocate_id();

/**
 * @brief Creates a new process structure with minimal initialization
 * @return A pointer to the newly created process, or NULL on failure
 */
process_t* process_create_empty();

/**
 * @brief Creates a new process with the specified address space and entry point
 * @param address_space The virtual memory address space for the process
 * @param entry The entry point virtual address where the process should begin execution
 * @param user_stack The top of the user stack for the process
 * @return A pointer to the newly created process, or NULL on failure
 */
process_t* process_create(vm_address_space_t* address_space, virt_addr_t entry, virt_addr_t user_stack);
