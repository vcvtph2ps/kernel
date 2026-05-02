#include <arch/cpu_local.h>
#include <arch/msr.h>
#include <common/sched/sched.h>
#include <common/sched/thread.h>
#include <common/userspace/syscall.h>
#include <log.h>

syscall_ret_t syscall_sys_exit(syscall_args_t args) {
    uint64_t exit_code = args.arg1;
    LOG_STRC("syscall_sys_exit: exit_code=%lu\n", exit_code);
    sched_yield(THREAD_STATE_DEAD);
    return SYSCALL_RET_VALUE(0);
}

syscall_ret_t syscall_sys_set_tcb(syscall_args_t args) {
    uint64_t pointer = args.arg1;
    LOG_STRC("syscall_sys_set_tcb: pointer=0x%lx\n", pointer);
    arch_msr_write_msr(IA32_FS_BASE_MSR, pointer); // @safety: what safety lol
    return SYSCALL_RET_VALUE(0);
}

syscall_ret_t syscall_sys_debug_log(syscall_args_t args) {
    uint64_t buf = args.arg1;
    uint64_t count = args.arg2;
    LOG_STRC("syscall_sys_debug_log: buf=0x%lx, count=%lu\n", buf, count);

    if(LOG_LEVEL_MIN > LOG_LEVEL_DBGL) { return SYSCALL_RET_VALUE(0); }
    if(count == 0) { return SYSCALL_RET_VALUE(0); }
    if(!userspace_validate_buffer(CPU_LOCAL_GET_CURRENT_THREAD()->common.process, buf, count)) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT); }

    LOG_DBGL("%.*s\n", (int) count, (const char*) buf);
    return SYSCALL_RET_VALUE(0);
}
