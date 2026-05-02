#pragma once

#include <ldr/elfldr.h>
#include <memory/memory.h>

/**
 * @brief Initializes the user stack for a process according to the sysv abi, pushes the argc, argv, envp, and auxv values on the stack.
 * @param process The process for which to initialize the user stack
 * @param user_stack_top The virtual address of the top of the user stack
 * @param loader_info The elf loader info for the executable being loaded, used to populate the auxv values on the stack
 * @return The virtual address of the bottom of the initialized stack (the new stack pointer after pushing the initial stack frame)
 */
virt_addr_t sysv_user_stack_init(vm_address_space_t* address_space, virt_addr_t user_stack_top, elfldr_elf_loader_info_t* loader_info);
