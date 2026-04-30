#include <log.h>
#include <string.h>

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0

// Compile nanoprintf in this translation unit.
#define NANOPRINTF_IMPLEMENTATION
#include <common/arch.h>
#include <common/boot/bootloader.h>
#include <common/io.h>
#include <flanterm.h>
#include <flanterm_backends/fb.h>
#include <lib/spinlock.h>
#include <nanoprintf/nanoprintf.h>

static struct flanterm_context* g_ft_ctx = nullptr;

void sink_debug(char* c) {
    while(*c != '\0') {
        arch_debug_putc(*c);
        c++;
    }
}


void sink_flanterm(char* c) {
    while(*c != '\0') {
        if(*c == '\n') {
            flanterm_write(g_ft_ctx, "\r\n", 2);
            c++;
            continue;
        }
        flanterm_write(g_ft_ctx, c, 1);
        c++;
    }
}

void log_init(void) {
    if(g_bootloader_info.framebuffer_count == 0) {
        sink_debug("No framebuffer found!\n");
        return;
    }

    bootloader_framebuffer_info_t framebuffer;
    if(!bootloader_get_framebuffer(0, &framebuffer)) {
        sink_debug("Failed to get framebuffer info!\n");
        return;
    }

    g_ft_ctx = flanterm_fb_init(
        nullptr,
        nullptr,
        (uint32_t*) framebuffer.vaddr,
        framebuffer.width,
        framebuffer.height,
        framebuffer.pitch,
        framebuffer.masks.red_mask_size,
        framebuffer.masks.red_mask_shift,
        framebuffer.masks.green_mask_size,
        framebuffer.masks.green_mask_shift,
        framebuffer.masks.blue_mask_size,
        framebuffer.masks.blue_mask_shift,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        0,
        0,
        1,
        0,
        0,
        0,
        FLANTERM_FB_ROTATE_0
    );

    if(g_ft_ctx == nullptr) { arch_panic("Failed to setup flanterm context"); }
}

static spinlock_no_int_t g_log_lock = {};

int nl_vprintf(const char* fmt, va_list val) {
    char buffer[512];
    const int rv = npf_vsnprintf(buffer, 512, fmt, val);
    sink_debug(buffer);
    if(g_ft_ctx != nullptr) { sink_flanterm(buffer); }
    return rv;
}

int nl_printf(const char* fmt, ...) {
    va_list val;
    va_start(val, fmt);
    const int rv = nl_vprintf(fmt, val);
    va_end(val);
    return rv;
}

int vprintf(const char* fmt, va_list val) {
    char buffer[512];
    const int rv = npf_vsnprintf(buffer, 512, fmt, val);
    uint64_t interrupt_state = spinlock_noint_lock(&g_log_lock);
    sink_debug(buffer);
    if(g_ft_ctx != nullptr) { sink_flanterm(buffer); }
    spinlock_noint_unlock(&g_log_lock, interrupt_state);
    return rv;
}

void log_print_nolock(log_level_t level, const char* fmt, ...) {
    if(level < LOG_LEVEL_MIN) { return; }
    va_list val;
    va_start(val, fmt);
    nl_vprintf(fmt, val);
    va_end(val);
}

void log_print(log_level_t level, const char* fmt, ...) {
    if(level < LOG_LEVEL_MIN) { return; }
    va_list val;
    va_start(val, fmt);
    vprintf(fmt, val);
    va_end(val);
}
