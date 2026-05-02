#pragma once
#include <common/userspace/syscall.h>

void sys_futex_init();
void sys_futex_check_timeouts();

// NOLINTBEGIN
syscall_ret_t syscall_sys_futex(syscall_args_t args);
// NOLINTEND
