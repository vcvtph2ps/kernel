#include <arch/cpu_local.h>
#include <buffer.h>
#include <common/sched/sched.h>
#include <common/sched/thread.h>
#include <common/tty.h>
#include <common/userspace/process.h>
#include <lib/helpers.h>
#include <list.h>
#include <memory/heap.h>
#include <spinlock.h>
#include <string.h>
#include <wait_queue.h>

tty_t* g_tty;

tty_t* tty_init() {
    tty_t* tty = heap_alloc(sizeof(tty_t));
    tty->lock = SPINLOCK_NO_DW_INIT;

    tty->input_buffer_capacity = 64;
    tty->input_buffer = heap_alloc(tty->input_buffer_capacity);
    tty->input_buffer_size = 0;

    tty->wait_queue = WAIT_QUEUE_INIT;
    tty->echo = true;
    return tty;
}

void tty_put_byte(tty_t* tty, uint8_t byte) {
    spinlock_nodw_lock(&tty->lock);

    if(tty->input_buffer_size < tty->input_buffer_capacity) { tty->input_buffer[tty->input_buffer_size] = byte; }

    if(byte == '\n') {
        if(tty->input_buffer_size < tty->input_buffer_capacity) tty->input_buffer_size++;
        wait_queue_wake_one(&tty->wait_queue);
    } else if(byte == '\b') {
        if(tty->input_buffer_size > 0) { tty->input_buffer_size--; }
    } else if(byte == ('u' & 0x1f)) {
        tty->input_buffer_size = 0;
    } else {
        if(tty->input_buffer_size < tty->input_buffer_capacity) tty->input_buffer_size++;
    }
    spinlock_nodw_unlock(&tty->lock);
    if(tty->echo) nl_printf("%c", byte);
}

void tty_wait_for(tty_t* tty) {
    while(true) {
        spinlock_nodw_lock(&tty->lock);
        if(tty->input_buffer_size != 0) {
            // @note: we leak the lock here on purpose
            break;
        }
        spinlock_nodw_unlock(&tty->lock);
        wait_queue_join(&tty->wait_queue);
    }
}

char* tty_read(tty_t* tty, size_t* size) {
    if(size == nullptr) { return nullptr; }
    // @todo: check if the current process is the process that owns this tty
    spinlock_nodw_lock(&tty->lock);
    if(tty->input_buffer_size == 0) {
        spinlock_nodw_unlock(&tty->lock);
        tty_wait_for(tty);
    }

    // @note: here we should have the tty lock
    // either we grabbed it and there is already data in the buffer, or we waited for data to be put in the buffer

    char* data = heap_alloc(tty->input_buffer_size + 1);
    memcpy(data, tty->input_buffer, tty->input_buffer_size);
    data[tty->input_buffer_size] = '\0';
    *size = tty->input_buffer_size;

    tty->input_buffer_size = 0;
    spinlock_nodw_unlock(&tty->lock);
    return data;
}
