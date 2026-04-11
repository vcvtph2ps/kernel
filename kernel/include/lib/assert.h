#pragma once
#include <common/arch.h>
#include <log.h>

// NOLINTBEGIN
#define assert(expr)                                                                                                                               \
    do {                                                                                                                                           \
        if(!(expr)) {                                                                                                                              \
            log_print_nolock(LOG_LEVEL_FAIL, LOG_COLORIZE("fail | ", "91") "Assertion failed: %s, file %s, line %d\n", #expr, __FILE__, __LINE__); \
            arch_die();                                                                                                                            \
        }                                                                                                                                          \
    } while(0)

#define assertf(expr, fmt, ...)                                                                                                                                            \
    do {                                                                                                                                                                   \
        if(!(expr)) {                                                                                                                                                      \
            log_print_nolock(LOG_LEVEL_FAIL, LOG_COLORIZE("fail | ", "91") "Assertion failed: %s, file %s, line %d: " fmt "\n", #expr, __FILE__, __LINE__, ##__VA_ARGS__); \
            arch_die();                                                                                                                                                    \
        }                                                                                                                                                                  \
    } while(0)

#define EXPECT_LIKELY(V) (__builtin_expect(!!(V), 1))
#define EXPECT_UNLIKELY(V) (__builtin_expect(!!(V), 0))

#define ASSERT_UNREACHABLE()                                                                                                   \
    do {                                                                                                                       \
        log_print_nolock(LOG_LEVEL_FAIL, LOG_COLORIZE("fail | ", "91") "Unreachable, file %s, line %d\n", __FILE__, __LINE__); \
        arch_die();                                                                                                            \
    } while(0)
// NOLINTEND
