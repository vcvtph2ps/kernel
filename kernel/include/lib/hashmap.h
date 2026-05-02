#pragma once

#include <lib/hasher.h>
#include <lib/list.h>
#include <stddef.h>

typedef struct {
    list_node_t list_node;
} hashmap_node_t;

typedef struct {
    list_t* buckets;
    size_t bucket_count;
} hashmap_t;

/**
 * @brief Initialise a hashmap, allocating the bucket array on the heap
 * @param map Hashmap to initialise
 * @param bucket_count Number of buckets
 * @return true on success, false if the heap allocation failed
 */
bool hashmap_init(hashmap_t* map, size_t bucket_count);

/**
 * @brief Insert a node into the hashmap
 * @param map Target hashmap
 * @param hash Precomputed hash of the key
 * @param node Node to insert (must not already be in a hashmap)
 */
void hashmap_insert(hashmap_t* map, hash_t hash, hashmap_node_t* node);

/**
 * @brief Remove a node from the hashmap
 * @param map Target hashmap
 * @param hash Precomputed hash of the key
 * @param node Node to remove
 */
void hashmap_remove(hashmap_t* map, hash_t hash, hashmap_node_t* node);

/**
 * @brief Return the bucket list for a given hash so the caller can iterate and
 * compare keys
 * @param map Target hashmap
 * @param hash Precomputed hash of the key
 * @return Pointer to the bucket's list_t
 */
list_t* hashmap_bucket(hashmap_t* map, hash_t hash);
