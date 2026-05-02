#pragma once
#include <common/userspace/syscall.h>

// NOLINTBEGIN
syscall_ret_t syscall_sys_open(syscall_args_t args);
syscall_ret_t syscall_sys_read(syscall_args_t args);
syscall_ret_t syscall_sys_write(syscall_args_t args);
syscall_ret_t syscall_sys_seek(syscall_args_t args);
syscall_ret_t syscall_sys_close(syscall_args_t args);
syscall_ret_t syscall_sys_close(syscall_args_t args);
syscall_ret_t syscall_sys_is_a_tty(syscall_args_t args);
// NOLINTEND
