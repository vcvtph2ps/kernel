#include <common/sched/sched.h>
#include <common/sched/thread.h>
#include <common/userspace/syscall.h>
#include <log.h>

syscall_ret_t syscall_sys_exit(uint64_t exit_code) {
    LOG_STRC("syscall_sys_exit: exit_code=%lu\n", exit_code);
    sched_yield(THREAD_STATE_DEAD);
    return SYSCALL_RET_VALUE(0);
}
