#pragma once

#include <stdint.h>

typedef uint64_t hash_t; // NOLINT

typedef struct {
    hash_t hash;
} hasher_t;

/**
 * @brief Create a new hasher with the initial FNV offset basis
 */
static inline hasher_t hasher_new() {
    return (hasher_t) { .hash = 0xcbf29ce484222325ULL };
}

/**
 * @brief Update the hasher with a new value using FNV-1a
 * @param hasher Hasher to update
 * @param value Value to hash (must be the same for the same input data across runs)
 */
static inline void hasher_hash(hasher_t* hasher, uint64_t value) {
    hasher->hash ^= value;
    hasher->hash *= 0x100000001b3ULL;
}

/**
 * @brief Finalize the hasher and return the resulting hash value
 * @param hasher Hasher to finalize
 * @return Final hash value
 */
static inline hash_t hasher_finalize(hasher_t hasher) {
    return hasher.hash;
}
