#include <arch/cpu_local.h>
#include <common/time.h>
#include <common/userspace/structs.h>
#include <common/userspace/syscalls/sys_clock.h>

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1

syscall_ret_t syscall_sys_get_clock(syscall_args_t args) {
    uintptr_t clock_type = args.arg1;
    uintptr_t timeout_struct = args.arg2;

    process_t* proc = CPU_LOCAL_GET_CURRENT_THREAD()->common.process;


    if(!userspace_validate_buffer(proc, timeout_struct, sizeof(structs_timespec_t))) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT);
    }

    structs_timespec_t out_timespec;

    switch(clock_type) {
        case CLOCK_REALTIME: {
            uint64_t ns = time_realtime_ns();
            out_timespec.tv_sec = ns / 1000000000ULL;
            out_timespec.tv_nsec = ns % 1000000000ULL;
            break;
        }
        case CLOCK_MONOTONIC: {
            uint64_t ns = time_monotonic_ns();
            out_timespec.tv_sec = ns / 1000000000ULL;
            out_timespec.tv_nsec = ns % 1000000000ULL;
            break;
        }
        default: return SYSCALL_RET_ERROR(SYSCALL_ERROR_INVAL);
    }

    vm_copy_to(proc->address_space, timeout_struct, &out_timespec, sizeof(structs_timespec_t));
    return SYSCALL_RET_VALUE(0);
}
