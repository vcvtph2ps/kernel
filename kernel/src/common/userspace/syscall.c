#include <arch/msr.h>
#include <assert.h>
#include <common/sched/sched.h>
#include <common/sched/thread.h>
#include <common/userspace/syscall.h>
#include <common/userspace/syscalls/sys_clock.h>
#include <common/userspace/syscalls/sys_futex.h>
#include <common/userspace/syscalls/sys_mem.h>
#include <common/userspace/syscalls/sys_proc.h>
#include <common/userspace/syscalls/sys_vfs.h>
#include <memory/ptm.h>

#define SYSTRACE_ENABLED LOG_LEVEL_MIN == LOG_LEVEL_STRC

typedef syscall_ret_t (*fn_syscall_handler_t)(syscall_args_t args);

typedef struct {
    fn_syscall_handler_t handler;
} syscall_entry_t;

static syscall_entry_t g_syscall_table[SYSCALL_HIGHEST_NR];

const char* userspace_syscall_number_to_string(syscall_nr_t nr) {
    switch(nr) {
        case SYSCALL_SYS_EXIT:   return "SYS_EXIT";
        case SYSCALL_DEBUG_LOG:  return "SYS_DEBUG_LOG";
        case SYSCALL_TCB_SET:    return "SYS_TCB_SET";
        case SYSCALL_FORK:       return "SYS_FORK";
        case SYSCALL_VM_MAP:     return "SYS_VM_MAP";
        case SYSCALL_VM_UNMAP:   return "SYS_VM_UNMAP";
        case SYSCALL_VM_PROTECT: return "SYS_VM_PROTECT";
        case SYSCALL_OPEN:       return "SYS_OPEN";
        case SYSCALL_CLOSE:      return "SYS_CLOSE";
        case SYSCALL_READ:       return "SYS_READ";
        case SYSCALL_WRITE:      return "SYS_WRITE";
        case SYSCALL_SEEK:       return "SYS_SEEK";
        case SYSCALL_ISATTY:     return "SYS_ISATTY";
        case SYSCALL_FUTEX:      return "SYS_FUTEX";
        default:                 return "Unknown syscall";
    }
}

const char* userspace_syscall_ret_to_string(syscall_ret_t ret) {
    if(!ret.is_error) {
        return "SUCCESS";
    }
    switch(ret.err) {
        case SYSCALL_ERROR_NOENT: return "ERROR_NOENT";
        case SYSCALL_ERROR_NOMEM: return "ERROR_NOMEM";
        case SYSCALL_ERROR_FAULT: return "ERROR_FAULT";
        case SYSCALL_ERROR_INVAL: return "ERROR_INVAL";
        case SYSCALL_ERROR_SPIPE: return "ERROR_SPIPE";
        case SYSCALL_ERROR_ROFS:  return "ERROR_ROFS";
        case SYSCALL_ERROR_RANGE: return "ERROR_RANGE";
        case SYSCALL_ERROR_BADFD: return "ERROR_BADFD";
        case SYSCALL_ERROR_AGAIN: return "ERROR_AGAIN";
        default:                  arch_panic("Unknown syscall error code: %lu", ret.err);
    }
}

syscall_ret_t syscall_sys_invalid(syscall_args_t args) {
    LOG_INFO("Invalid syscall \"%s\" (0x%llx) invoked with args: %lu, %lu, %lu, %lu, %lu, %lu\n", userspace_syscall_number_to_string(args.syscall_nr), args.syscall_nr, args.arg1, args.arg2, args.arg3, args.arg4, args.arg5, args.arg6);
    (void) args;

    assert(false);
    return SYSCALL_RET_ERROR(SYSCALL_ERROR_INVAL);
}

syscall_ret_t dispatch_syscall(syscall_frame_t* frame) {
    uint64_t syscall_nr = frame->regs.rax;
    assert(syscall_nr < SYSCALL_HIGHEST_NR);
    syscall_entry_t entry = g_syscall_table[syscall_nr];

    syscall_args_t args;
    args.syscall_nr = syscall_nr;
    args.arg1 = frame->regs.rdi;
    args.arg2 = frame->regs.rsi;
    args.arg3 = frame->regs.rdx;
    args.arg4 = frame->regs.r10;
    args.arg5 = frame->regs.r8;
    args.arg6 = frame->regs.r9;
    args.frame = frame;

    return entry.handler(args);
}

#define SYSCALL_DISPATCHER(nr, __handler) g_syscall_table[nr].handler = __handler;

void arch_syscall_init();

void syscall_init() {
    arch_syscall_init();
    sys_futex_init();

    for(size_t i = 0; i < SYSCALL_HIGHEST_NR; i++) {
        g_syscall_table[i].handler = syscall_sys_invalid;
    }

    SYSCALL_DISPATCHER(SYSCALL_SYS_EXIT, syscall_sys_exit);
    SYSCALL_DISPATCHER(SYSCALL_DEBUG_LOG, syscall_sys_debug_log);
    SYSCALL_DISPATCHER(SYSCALL_TCB_SET, syscall_sys_set_tcb);

    SYSCALL_DISPATCHER(SYSCALL_VM_MAP, syscall_sys_vm_map);
    SYSCALL_DISPATCHER(SYSCALL_VM_UNMAP, syscall_sys_vm_unmap);
    SYSCALL_DISPATCHER(SYSCALL_VM_PROTECT, syscall_sys_vm_protect);

    SYSCALL_DISPATCHER(SYSCALL_OPEN, syscall_sys_open);
    SYSCALL_DISPATCHER(SYSCALL_CLOSE, syscall_sys_close);
    SYSCALL_DISPATCHER(SYSCALL_READ, syscall_sys_read);
    SYSCALL_DISPATCHER(SYSCALL_WRITE, syscall_sys_write);
    SYSCALL_DISPATCHER(SYSCALL_SEEK, syscall_sys_seek);
    SYSCALL_DISPATCHER(SYSCALL_ISATTY, syscall_sys_is_a_tty);
    SYSCALL_DISPATCHER(SYSCALL_GET_CWD, syscall_sys_get_cwd);
    SYSCALL_DISPATCHER(SYSCALL_STAT, syscall_sys_stat);
    SYSCALL_DISPATCHER(SYSCALL_STAT_AT, syscall_sys_stat_at);

    SYSCALL_DISPATCHER(SYSCALL_FUTEX, syscall_sys_futex);
    SYSCALL_DISPATCHER(SYSCALL_GET_CLOCK, syscall_sys_get_clock);
    SYSCALL_DISPATCHER(SYSCALL_GET_PROCESS_INFO, syscall_sys_get_process_info);
}
