#include <arch/msr.h>
#include <assert.h>
#include <common/sched/sched.h>
#include <common/sched/thread.h>
#include <common/userspace/syscall.h>
#include <common/userspace/syscalls/sys_proc.h>

#define SYSTRACE_ENABLED LOG_LEVEL_MIN == LOG_LEVEL_STRC

typedef syscall_ret_t (*fn_syscall_handler0_t)();
typedef syscall_ret_t (*fn_syscall_handler1_t)(uint64_t);
typedef syscall_ret_t (*fn_syscall_handler2_t)(uint64_t, uint64_t);
typedef syscall_ret_t (*fn_syscall_handler3_t)(uint64_t, uint64_t, uint64_t);
typedef syscall_ret_t (*fn_syscall_handler4_t)(uint64_t, uint64_t, uint64_t, uint64_t);
typedef syscall_ret_t (*fn_syscall_handler5_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
typedef syscall_ret_t (*fn_syscall_handler6_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

typedef struct {
    size_t num_params;
    union {
        fn_syscall_handler0_t handler0;
        fn_syscall_handler1_t handler1;
        fn_syscall_handler2_t handler2;
        fn_syscall_handler3_t handler3;
        fn_syscall_handler4_t handler4;
        fn_syscall_handler5_t handler5;
        fn_syscall_handler6_t handler6;
    } handlers;
} syscall_entry_t;

#define MAX_SYSCALL_NUMBER 16

static syscall_entry_t g_syscall_table[MAX_SYSCALL_NUMBER];

const char* userspace_syscall_number_to_string(syscall_nr_t nr) {
    switch(nr) {
        case SYSCALL_SYS_EXIT: return "SYS_EXIT";
        default:               return "Unknown syscall";
    }
}

const char* userspace_syscall_ret_to_string(syscall_ret_t ret) {
    if(!ret.is_error) { return "SUCCESS"; }
    switch(ret.err) {
        case SYSCALL_ERROR_INVAL: return "ERROR_INVAL";
        default:                  arch_panic("Unknown syscall error code: %lu", ret.err);
    }
}

syscall_ret_t syscall_sys_invalid(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    (void) arg1;
    (void) arg2;
    (void) arg3;
    (void) arg4;
    (void) arg5;
    (void) arg6;

    assert(false);
    return SYSCALL_RET_ERROR(SYSCALL_ERROR_INVAL);
}

syscall_ret_t dispatch_syscall(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6, syscall_nr_t syscall_nr) {
    syscall_entry_t entry = g_syscall_table[syscall_nr];
    assert(entry.num_params <= 6 && "syscall entry has too many parameters");

    syscall_ret_t ret_value;

    switch(entry.num_params) {
        case 0:  ret_value = entry.handlers.handler0(); break;
        case 1:  ret_value = entry.handlers.handler1(arg1); break;
        case 2:  ret_value = entry.handlers.handler2(arg1, arg2); break;
        case 3:  ret_value = entry.handlers.handler3(arg1, arg2, arg3); break;
        case 4:  ret_value = entry.handlers.handler4(arg1, arg2, arg3, arg4); break;
        case 5:  ret_value = entry.handlers.handler5(arg1, arg2, arg3, arg4, arg5); break;
        case 6:  ret_value = entry.handlers.handler6(arg1, arg2, arg3, arg4, arg5, arg6); break;
        default: __builtin_unreachable();
    }

    return ret_value;
}

#define SYSCALL_DISPATCHER(nr, __handler, __num_params)             \
    g_syscall_table[nr].num_params = __num_params;                  \
    g_syscall_table[nr].handlers.handler##__num_params = __handler;

void arch_syscall_init();

void syscall_init() {
    arch_syscall_init();

    for(size_t i = 0; i < MAX_SYSCALL_NUMBER; i++) {
        g_syscall_table[i].num_params = 6;
        g_syscall_table[i].handlers.handler6 = syscall_sys_invalid;
    }

    SYSCALL_DISPATCHER(SYSCALL_SYS_EXIT, syscall_sys_exit, 1);
}
