#pragma once

#include <common/userspace/process.h>
#include <memory/memory.h>
#include <stddef.h>
#include <stdint.h>

typedef enum : uint64_t {
    SYSCALL_SYS_EXIT = 0,
    SYSCALL_DEBUG_LOG = 1,
    SYSCALL_TCB_SET = 2,

    SYSCALL_VM_MAP = 10,
    SYSCALL_VM_UNMAP = 11,
    SYSCALL_VM_PROTECT = 12,

    SYSCALL_OPEN = 20,
    SYSCALL_CLOSE = 21,
    SYSCALL_READ = 22,
    SYSCALL_WRITE = 23,
    SYSCALL_SEEK = 24,
    SYSCALL_ISATTY = 25,
    SYSCALL_GET_CWD = 26,
    SYSCALL_STAT = 27,
    SYSCALL_STAT_AT = 28,

    SYSCALL_FUTEX = 30,
    SYSCALL_GET_CLOCK = 31,
    SYSCALL_GET_PROCESS_INFO = 32,

    SYSCALL_HIGHEST_NR
} syscall_nr_t;

typedef enum : int64_t {
    SYSCALL_ERROR_AGAIN = 11, // Try again (EAGAIN)
    SYSCALL_ERROR_NOENT = 2, // No such file or directory
    SYSCALL_ERROR_NOMEM = 12, // Out of memory
    SYSCALL_ERROR_FAULT = 14, // Bad address
    SYSCALL_ERROR_INVAL = 22, // Invalid argument
    SYSCALL_ERROR_NOTTY = 25, // Not a TTY
    SYSCALL_ERROR_SPIPE = 29, // Illegal seek
    SYSCALL_ERROR_ROFS = 30, // Read-only file system
    SYSCALL_ERROR_RANGE = 34, // Out of range
    SYSCALL_ERROR_BADFD = 77, // Bad file descriptor
    SYSCALL_ERROR_TIMEDOUT = 110, // Connection timed out (ETIMEDOUT)
} syscall_err_t;

typedef struct [[gnu::packed]] {
    union {
        syscall_err_t err;
        uint64_t value;
    };
    uint64_t is_error;
} syscall_ret_t;

static_assert(sizeof(syscall_ret_t) == 16, "syscall_ret_t must be 16 bytes");

#define SYSCALL_RET_ERROR(err_code) ((syscall_ret_t) { .is_error = true, .err = (err_code) })
#define SYSCALL_RET_VALUE(val) ((syscall_ret_t) { .is_error = false, .value = (val) })

// @note: user rip is in rcx. user rflags is in r11
typedef struct {
    uint64_t user_rsp;
    arch_interrupts_regs_t regs;
} syscall_frame_t;

typedef struct {
    uint64_t syscall_nr;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
    uint64_t arg6;
    syscall_frame_t* frame;
} syscall_args_t;

/**
 * @brief Initializes the syscall dispatch system
 */
void syscall_init();

/**
 * @brief Validates that the given userspace buffer is a valid pointer to a buffer of the specified size in the given process's address space
 * @param process The process whose address space to validate against
 * @param buf The virtual address of the start of the buffer
 * @param count The size of the buffer in bytes
 */
bool userspace_validate_buffer(process_t* process, virt_addr_t buf, size_t count); // NOLINT
