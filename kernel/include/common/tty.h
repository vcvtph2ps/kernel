#include <lib/buffer.h>
#include <lib/list.h>
#include <lib/wait_queue.h>
#include <spinlock.h>
#include <stdint.h>

typedef struct {
    spinlock_no_dw_t lock;

    uint8_t* input_buffer;
    size_t input_buffer_size;
    size_t input_buffer_capacity;

    wait_queue_t wait_queue;
    bool echo;
} tty_t;

extern tty_t* g_tty;

/**
 * @brief Initializes a TTY instance.
 */
tty_t* tty_init();

/**
 * @brief Puts a byte into the TTY's input buffer, waking up any waiting threads if a newline is received.
 * @param tty The TTY instance to put the byte into.
 * @param byte The byte to put into the TTY's input buffer.
 */
void tty_put_byte(tty_t* tty, uint8_t byte);

/**
 * @brief Waits until there is at least one byte in the TTY's input buffer. The caller must hold the TTY lock before calling this function.
 * @param tty The TTY instance to wait on.
 */
char* tty_read(tty_t* tty, size_t* size);
