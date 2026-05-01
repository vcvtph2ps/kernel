#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum : uint64_t {
    SYSCALL_SYS_EXIT = 0,
} syscall_nr_t;

typedef enum : int64_t {
    SYSCALL_ERROR_INVAL = 22 // Invalid argument
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

/**
 * @brief Initializes the syscall dispatch system
 */
void syscall_init();
