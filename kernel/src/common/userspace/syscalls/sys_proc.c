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

#define SYSCALL_GET_PROCESS_INFO_PID 0
#define SYSCALL_GET_PROCESS_INFO_PPID 1
#define SYSCALL_GET_PROCESS_INFO_GID 2
#define SYSCALL_GET_PROCESS_INFO_EGID 3
#define SYSCALL_GET_PROCESS_INFO_UID 4
#define SYSCALL_GET_PROCESS_INFO_EUID 5
#define SYSCALL_GET_PROCESS_INFO_GET_PGID 6
#define SYSCALL_GET_PROCESS_INFO_SET_PGID 7

syscall_ret_t internal_get_pgid(uint64_t pid) {
    process_t* process;
    if(pid == 0) {
        process = CPU_LOCAL_GET_CURRENT_THREAD()->common.process;
    } else {
        process = process_get_by_pid(pid);
    }

    if(process == nullptr) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_INVAL); }

    return SYSCALL_RET_VALUE(process->process_group_pid);
}

syscall_ret_t internal_set_pgid(uint64_t pid, uint64_t pgid) {
    process_t* process;
    if(pid == 0) {
        process = CPU_LOCAL_GET_CURRENT_THREAD()->common.process;
    } else {
        process = process_get_by_pid(pid);
    }

    if(process == nullptr) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_INVAL); }

    // @todo: make this not fucking lazy
    process->process_group_pid = pgid;
    return SYSCALL_RET_VALUE(0);
}

syscall_ret_t syscall_sys_get_process_info(uint64_t query, uint64_t param1, uint64_t param2) {
    // @todo: process group id is lazy as fuck and not posix compliant
    switch(query) {
        case SYSCALL_GET_PROCESS_INFO_PID:      return SYSCALL_RET_VALUE(CPU_LOCAL_GET_CURRENT_THREAD()->common.process->pid);
        case SYSCALL_GET_PROCESS_INFO_PPID:     return SYSCALL_RET_VALUE(0);
        case SYSCALL_GET_PROCESS_INFO_GID:      return SYSCALL_RET_VALUE(0);
        case SYSCALL_GET_PROCESS_INFO_EGID:     return SYSCALL_RET_VALUE(0);
        case SYSCALL_GET_PROCESS_INFO_UID:      return SYSCALL_RET_VALUE(0);
        case SYSCALL_GET_PROCESS_INFO_EUID:     return SYSCALL_RET_VALUE(0);
        case SYSCALL_GET_PROCESS_INFO_GET_PGID: return internal_get_pgid(param1);
        case SYSCALL_GET_PROCESS_INFO_SET_PGID: return internal_set_pgid(param1, param2);
        default:                                return SYSCALL_RET_ERROR(SYSCALL_ERROR_INVAL);
    }
}
