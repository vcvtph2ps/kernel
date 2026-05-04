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
static spinlock_no_int_t g_log_lock = {};

#define MAX_LOG_SINKS 8
static log_sink_t g_sinks[MAX_LOG_SINKS];
static size_t g_sink_count = 0;

void sink_debug(const char* c, void* ctx) {
    (void) ctx;
    while(*c != '\0') {
        arch_debug_putc(*c);
        c++;
    }
}


void sink_flanterm(const char* c, void* ctx) {
    struct flanterm_context* ft_ctx = (struct flanterm_context*) ctx;
    if(!ft_ctx) return;
    while(*c != '\0') {
        if(*c == '\n') {
            flanterm_write(ft_ctx, "\r\n", 2);
            c++;
            continue;
        }
        flanterm_write(ft_ctx, c, 1);
        c++;
    }
}

bool log_add_sink(const log_sink_t* sink) {
    uint64_t state = spinlock_noint_lock(&g_log_lock);
    if(g_sink_count >= MAX_LOG_SINKS) {
        spinlock_noint_unlock(&g_log_lock, state);
        return false;
    }
    g_sinks[g_sink_count++] = *sink;
    spinlock_noint_unlock(&g_log_lock, state);
    return true;
}

static void dispatch_to_sinks(log_level_t level, const char* buffer) {
    for(size_t i = 0; i < g_sink_count; i++) {
        if(level >= g_sinks[i].min_level && g_sinks[i].write != nullptr) {
            g_sinks[i].write(buffer, g_sinks[i].ctx);
        }
    }
}

void log_init(void) {
    log_sink_t debug_sink = { .min_level = LOG_LEVEL_STRC, .write = sink_debug, .ctx = nullptr };
    log_add_sink(&debug_sink);
}

void log_init_framebuffer(void) {
    if(g_bootloader_info.framebuffer_count == 0) {
        dispatch_to_sinks(LOG_LEVEL_WARN, "No framebuffer found!\n");
        return;
    }

    bootloader_framebuffer_info_t framebuffer;
    if(!bootloader_get_framebuffer(0, &framebuffer)) {
        dispatch_to_sinks(LOG_LEVEL_WARN, "Failed to get framebuffer info!\n");
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

    if(g_ft_ctx == nullptr) {
        arch_panic("Failed to setup flanterm context");
    }

    log_sink_t ft_sink = { .min_level = LOG_LEVEL_INFO, .write = sink_flanterm, .ctx = g_ft_ctx };
    log_add_sink(&ft_sink);
}

int nl_vprintf(const char* fmt, va_list val) {
    char buffer[512];
    const int rv = npf_vsnprintf(buffer, 512, fmt, val);
    dispatch_to_sinks(LOG_LEVEL_INFO, buffer);
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
    dispatch_to_sinks(LOG_LEVEL_INFO, buffer);
    spinlock_noint_unlock(&g_log_lock, interrupt_state);
    return rv;
}

void log_print_nolock(log_level_t level, const char* fmt, ...) {
    if(level < LOG_LEVEL_MIN) {
        return;
    }
    char buffer[512];
    va_list val;
    va_start(val, fmt);
    npf_vsnprintf(buffer, 512, fmt, val);
    va_end(val);
    dispatch_to_sinks(level, buffer);
}

void log_print(log_level_t level, const char* fmt, ...) {
    if(level < LOG_LEVEL_MIN) {
        return;
    }
    char buffer[512];
    va_list val;
    va_start(val, fmt);
    npf_vsnprintf(buffer, 512, fmt, val);
    va_end(val);

    uint64_t interrupt_state = spinlock_noint_lock(&g_log_lock);
    dispatch_to_sinks(level, buffer);
    spinlock_noint_unlock(&g_log_lock, interrupt_state);
}

uint64_t log_lock() {
    return spinlock_noint_lock(&g_log_lock);
}

void log_unlock(uint64_t interrupt_state) {
    spinlock_noint_unlock(&g_log_lock, interrupt_state);
}
