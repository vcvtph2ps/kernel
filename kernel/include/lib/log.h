#pragma once
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    LOG_LEVEL_STRC = 0,
    LOG_LEVEL_DBGL,
    LOG_LEVEL_INFO,
    LOG_LEVEL_OKAY,
    LOG_LEVEL_WARN,
    LOG_LEVEL_FAIL,
} log_level_t;

#define LOG_LEVEL_MIN LOG_LEVEL_STRC

typedef void (*log_sink_func_t)(const char* msg, void* ctx);

typedef struct log_sink {
    log_level_t min_level;
    log_sink_func_t write;
    void* ctx;
} log_sink_t;

/**
 * @brief Registers a new log sink dynamically.
 * @return True if the sink was added successfully, false otherwise (e.g. if too many sinks).
 */
bool log_add_sink(const log_sink_t* sink);

/**
 * @brief Initializes the logging system. This should be called once during early initialization before any calls to log_print or the LOG_* macros.
 */
void log_init(void);

/**
 * @brief Initializes the logging system. This should be called once during early initialization before any calls to log_print or the LOG_* macros.
 */
void log_init_framebuffer(void);

/**
 * @brief Locks the log lock
 * @note Calling any locking functions until log_unlock will deadlock
 * @return The previous interrupt state before the lock was acquired
 */
[[nodiscard]] uint64_t log_lock(void);

/**
 * @brief Unlocks the log lock
 * @param interrupt_state The previous interrupt state returned by log_lock.
 */
void log_unlock(uint64_t interrupt_state);

/**
 * @brief Non-locking vprintf
 */
int nl_vprintf(const char* fmt, va_list val); // NOLINT

/**
 * @brief Non-locking printf
 */
int nl_printf(const char* fmt, ...); // NOLINT

/**
 * @brief Prints a log message with the specified log level and format string. This should only be called after log_init has been called.
 * @param log The log level to print the message with. Messages with a log level less than LOG_LEVEL_MIN will not be printed.
 * @param fmt The format string for the log message, followed by any additional arguments required by the format string.
 */
void log_print(log_level_t log, const char* fmt, ...);

/**
 * @brief Prints a log message  with the specified log level and format string. This should only be called after log_init has been called. without blocking at all
 * @param log The log level to print the message with. Messages with a log level less than LOG_LEVEL_MIN will not be printed.
 * @param fmt The format string for the log message, followed by any additional arguments required by the format string.
 */
void log_print_nolock(log_level_t log, const char* fmt, ...);

#define LOG_COLORIZE(text, color) "\x1b[1m\x1b[" color "m" text "\x1b[0m"

#define LOG_STRINGIZE(x) #x

#define LOG_FAIL(fmt, ...)                                                                                        \
    do { log_print(LOG_LEVEL_FAIL, LOG_COLORIZE("fail | ", "91") "%s: " fmt, __func__, ##__VA_ARGS__); } while(0)

#define LOG_WARN(fmt, ...)                                                                                        \
    do { log_print(LOG_LEVEL_WARN, LOG_COLORIZE("warn | ", "93") "%s: " fmt, __func__, ##__VA_ARGS__); } while(0)

#define LOG_OKAY(fmt, ...)                                                                                        \
    do { log_print(LOG_LEVEL_OKAY, LOG_COLORIZE("okay | ", "92") "%s: " fmt, __func__, ##__VA_ARGS__); } while(0)

#define LOG_INFO(fmt, ...)                                                                                        \
    do { log_print(LOG_LEVEL_INFO, LOG_COLORIZE("info | ", "96") "%s: " fmt, __func__, ##__VA_ARGS__); } while(0)

#define LOG_DBGL(fmt, ...)                                                                                        \
    do { log_print(LOG_LEVEL_DBGL, LOG_COLORIZE("dbgl | ", "34") "%s: " fmt, __func__, ##__VA_ARGS__); } while(0)

#define LOG_STRC(fmt, ...)                                                                                        \
    do { log_print(LOG_LEVEL_STRC, LOG_COLORIZE("strc | ", "95") "%s: " fmt, __func__, ##__VA_ARGS__); } while(0)
