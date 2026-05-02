#pragma once
#include <common/userspace/syscall.h>

// NOLINTBEGIN
syscall_ret_t syscall_sys_exit(syscall_args_t args);
syscall_ret_t syscall_sys_set_tcb(syscall_args_t args);
syscall_ret_t syscall_sys_debug_log(syscall_args_t args);
syscall_ret_t syscall_sys_get_process_info(syscall_args_t args);
// NOLINTEND
