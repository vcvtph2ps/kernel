#include <lib/hashmap.h>
#include <lib/list.h>
#include <memory/heap.h>
#include <stdbool.h>

bool hashmap_init(hashmap_t* map, size_t bucket_count) {
    map->buckets = (list_t*) heap_alloc(sizeof(list_t) * bucket_count);
    if(map->buckets == nullptr) return false;
    map->bucket_count = bucket_count;
    for(size_t i = 0; i < bucket_count; i++) { map->buckets[i] = LIST_INIT; }
    return true;
}

void hashmap_insert(hashmap_t* map, uint64_t hash, hashmap_node_t* node) {
    list_push_back(&map->buckets[hash % map->bucket_count], &node->list_node);
}

void hashmap_remove(hashmap_t* map, uint64_t hash, hashmap_node_t* node) {
    list_node_delete(&map->buckets[hash % map->bucket_count], &node->list_node);
}

list_t* hashmap_bucket(hashmap_t* map, uint64_t hash) {
    return &map->buckets[hash % map->bucket_count];
}
