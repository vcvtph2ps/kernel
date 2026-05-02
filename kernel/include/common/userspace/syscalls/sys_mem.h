#pragma once
#include <common/userspace/syscall.h>

// NOLINTBEGIN
syscall_ret_t syscall_sys_vm_map(syscall_args_t args);
syscall_ret_t syscall_sys_vm_unmap(syscall_args_t args);
syscall_ret_t syscall_sys_vm_protect(syscall_args_t args);
// NOLINTEND
