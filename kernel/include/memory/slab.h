#pragma once
#include <lib/list.h>
#include <lib/spinlock.h>
#include <memory/memory.h>
#include <stddef.h>
#include <stdint.h>

#define SLAB_MAGAZINE_SIZE 32
#define SLAB_MAGAZINE_EXTRA (arch_get_core_count() * 4)

typedef struct slab slab_t;
typedef struct slab_magazine slab_magazine_t;

struct slab_magazine {
    void* objects[SLAB_MAGAZINE_SIZE];
    size_t rounds;

    list_node_t list_node;
};

typedef struct slab_cpu_cache {
    spinlock_no_dw_t lock;

    slab_magazine_t* primary;
    slab_magazine_t* secondary;
} slab_cpu_cache_t;

typedef struct slab_cache {
    const char* name;
    size_t object_size;
    size_t slab_size;
    size_t slab_align;

    list_node_t list_node;

    spinlock_no_dw_t slab_lock;
    list_t slab_full_list;
    list_t slab_partial_list;

    spinlock_no_dw_t magazine_lock;
    list_t magazine_full_list;
    list_t magazine_empty_list;

    bool cpu_cached;
    slab_cpu_cache_t cpu_cache[];
} slab_cache_t;

struct slab {
    virt_addr_t buffer;

    size_t free_count;
    void* free_list;

    slab_cache_t* cache;

    list_node_t list_node;
};

/**
 * @brief Initializes the slab allocator.
 * @note This function must be called before any other slab allocator functions are used. And should only called once
 */
void slab_cache_init();

/**
 * @brief Creates a new slab cache.
 * @param name The name of the slab cache, used for debugging purposes.
 * @param object_size The size of the objects that will be allocated from this cache.
 * @param alignment The alignment of the objects that will be allocated from this cache.
 * @return A pointer to the newly created slab cache, or NULL on failure.
 */
slab_cache_t* slab_cache_create(const char* name, size_t object_size, size_t alignment);

/**
 * @brief Destroys a slab cache and frees all associated resources.
 * @param cache The slab cache to destroy.
 * @note This function will free all slabs and magazines associated with the cache, and should only be called when the cache is no longer needed. After this function is called, the cache pointer should not be used.
 */
void slab_cache_destroy(slab_cache_t* cache);

/**
 * @brief Allocates an object from the slab cache.
 * @param cache The slab cache to allocate from.
 * @return A pointer to the allocated object, or NULL on failure.
 * @note The returned object is uninitialized and may contain garbage data. The caller is responsible for initializing the object before use. The caller is also responsible for freeing the object using slab_cache_free when it is no longer needed.
 */
void* slab_cache_alloc(slab_cache_t* cache);

/**
 * @brief Frees an object back to the slab cache.
 * @param cache The slab cache to free the object to.
 * @param object The object to free.
 * @note The object must have been allocated from the same slab cache. After this function is called, the object pointer should not be used.
 */
void slab_cache_free(slab_cache_t* cache, void* object);
