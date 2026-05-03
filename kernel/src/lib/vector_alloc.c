#include <lib/spinlock.h>
#include <lib/vector_alloc.h>
#include <stdbool.h>
#include <stdint.h>

static uint8_t g_vector_map[256] = { 0 };
static spinlock_t g_vector_lock = SPINLOCK_INIT;

#define VECTOR_START 0x20
#define VECTOR_END 0xEF

uint8_t vector_alloc_interrupt() {
    spinlock_lock(&g_vector_lock);

    for(int v = VECTOR_START; v <= VECTOR_END; ++v) {
        uint8_t vector = v;
        if(!g_vector_map[vector]) {
            g_vector_map[vector] = 1;
            spinlock_unlock(&g_vector_lock);
            return vector;
        }
    }
    spinlock_unlock(&g_vector_lock);
    return 0;
}

uint8_t vector_alloc_specific_interrupt(uint8_t vector) {
    if(vector < VECTOR_START || vector > VECTOR_END) { return -1; }
    spinlock_lock(&g_vector_lock);
    if(g_vector_map[vector]) {
        spinlock_unlock(&g_vector_lock);
        return 0;
    }
    g_vector_map[vector] = 1;
    spinlock_unlock(&g_vector_lock);
    return vector;
}

void vector_free_interrupt(uint8_t vector) {
    spinlock_lock(&g_vector_lock);
    g_vector_map[vector] = 0;
    spinlock_unlock(&g_vector_lock);
}
