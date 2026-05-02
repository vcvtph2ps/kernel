#pragma once
#include <stdarg.h>
#include <stddef.h>

typedef enum {
    LOG_LEVEL_STRC = 0,
    LOG_LEVEL_DBGL,
    LOG_LEVEL_INFO,
    LOG_LEVEL_OKAY,
    LOG_LEVEL_WARN,
    LOG_LEVEL_FAIL,
} log_level_t;

#define LOG_LEVEL_MIN LOG_LEVEL_STRC

/**
 * @brief Initializes the logging system. This should be called once during early initialization before any calls to log_print or the LOG_* macros.
 */
void log_init(void);

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

#define LOG_FAIL(fmt, ...)                                                                       \
    do { log_print(LOG_LEVEL_FAIL, LOG_COLORIZE("fail | ", "91") fmt, ##__VA_ARGS__); } while(0)

#define LOG_WARN(fmt, ...)                                                                       \
    do { log_print(LOG_LEVEL_WARN, LOG_COLORIZE("warn | ", "93") fmt, ##__VA_ARGS__); } while(0)

#define LOG_OKAY(fmt, ...)                                                                       \
    do { log_print(LOG_LEVEL_OKAY, LOG_COLORIZE("okay | ", "92") fmt, ##__VA_ARGS__); } while(0)

#define LOG_INFO(fmt, ...)                                                                       \
    do { log_print(LOG_LEVEL_INFO, LOG_COLORIZE("info | ", "96") fmt, ##__VA_ARGS__); } while(0)

#define LOG_DBGL(fmt, ...)                                                                       \
    do { log_print(LOG_LEVEL_DBGL, LOG_COLORIZE("dbgl | ", "34") fmt, ##__VA_ARGS__); } while(0)

#define LOG_STRC(fmt, ...)                                                                       \
    do { log_print(LOG_LEVEL_STRC, LOG_COLORIZE("strc | ", "95") fmt, ##__VA_ARGS__); } while(0)
