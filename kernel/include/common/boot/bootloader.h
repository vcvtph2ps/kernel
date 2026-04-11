#pragma once

#include <memory/memory.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    BOOTLOADER_NONE,
    BOOTLOADER_LIMINE,
    BOOTLOADER_TARTARUS
} bootloader_type_t;

typedef struct {
    phys_addr_t paddr;
    uintptr_t vaddr;
    size_t width;
    size_t height;
    size_t pitch;
    size_t bpp;
    size_t framebuffer_length;

    struct {
        uint8_t red_mask_size;
        uint8_t red_mask_shift;
        uint8_t green_mask_size;
        uint8_t green_mask_shift;
        uint8_t blue_mask_size;
        uint8_t blue_mask_shift;
    } masks;
} bootloader_framebuffer_info_t;

typedef enum {
    BOOTLOADER_MMAP_USABLE,
    BOOTLOADER_MMAP_RESERVED_MUST_MAP,
    BOOTLOADER_MMAP_RESERVED
} bootloader_mmap_entry_type_t;

typedef struct {
    bootloader_mmap_entry_type_t type;
    phys_addr_t base;
    size_t length;
} bootloader_mmap_entry_t;

typedef struct {
    bool can_boot;
    bool is_bsp;
} bootloader_cpu_info_t;

typedef struct {
    const char* path;
    void* address;
    size_t size;
} bootloader_module_t;

// NOLINTBEGIN
// @note the bootloader is **required** to update the address returned by bootloader_get_framebuffer_info to the new address whenever the framebuffer is remapped by the kernel
typedef void (*fn_bootloader_set_framebuffer_address)(size_t index, void* address);

// @return true if the framebuffer info was successfully retrieved, false if the index was out of bounds
typedef bool (*fn_bootloader_get_framebuffer_info)(size_t index, bootloader_framebuffer_info_t* info);
// @return true if the mmap entry was successfully retrieved, false if the index was out of bounds
typedef bool (*fn_bootloader_get_mmap_entry)(size_t index, bootloader_mmap_entry_t* info);
// @return true if the cpu info was successfully retrieved, false if the index was out of bounds
typedef bool (*fn_bootloader_get_cpu_info)(size_t index, bootloader_cpu_info_t* info);
// @return true if the module was successfully retrieved, false if the index was out of bounds
typedef bool (*fn_bootloader_get_module)(size_t index, bootloader_module_t* info);
// @return true if the ap was successfully **found**, not started
typedef bool (*fn_bootloader_start_ap)(size_t index, uint64_t arg);

typedef void (*fn_bootloader_map_kernel_segments)();
// NOLINTEND

typedef struct {
    bootloader_type_t type;
    uintptr_t hhdm_offset;
    phys_addr_t rdsp_base;
    size_t mmap_entry_count;
    size_t framebuffer_count;
    size_t cpu_count;
    size_t module_count;

    // you prob shouldn't call these functions
    fn_bootloader_get_framebuffer_info internal_get_framebuffer_info;
    fn_bootloader_set_framebuffer_address internal_set_framebuffer_address;
    fn_bootloader_get_mmap_entry internal_get_mmap_entry;
    fn_bootloader_map_kernel_segments internal_map_kernel_segments;
    fn_bootloader_get_cpu_info internal_get_cpu_info;
    fn_bootloader_get_module internal_get_module;
    fn_bootloader_start_ap internal_start_ap;
} bootloader_info_t;

extern bootloader_info_t g_bootloader_info;

// NOLINTBEGIN
#define TO_HHDM(x) (((uintptr_t) (x) + g_bootloader_info.hhdm_offset))
#define FROM_HHDM(x) (((uintptr_t) (x) - g_bootloader_info.hhdm_offset))
// NOLINTEND

/**
 * @brief Gets the framebuffer information for the framebuffer at the specified index.
 * @param index The index of the framebuffer to get information for. Must be less than g_bootloader_info.framebuffer_count.
 * @param info A pointer to a bootloader_framebuffer_info_t structure to receive the framebuffer information.
 * @return true if the framebuffer information was successfully retrieved, false if the index was out of bounds.
 */
bool bootloader_get_framebuffer(size_t index, bootloader_framebuffer_info_t* info);

/**
 * @brief Gets the memory map entry information for the entry at the specified index.
 * @param index The index of the memory map entry to get information for. Must be less than g_bootloader_info.mmap_entry_count.
 * @param info A pointer to a bootloader_mmap_entry_t structure to receive the memory map entry information.
 * @return true if the memory map entry information was successfully retrieved, false if the index was out of bounds.
 */
bool bootloader_get_mmap_entry(size_t index, bootloader_mmap_entry_t* info);

/**
 * @brief Gets the CPU information for the CPU at the specified index.
 * @param index The index of the CPU to get information for. Must be less than g_bootloader_info.cpu_count.
 * @param info A pointer to a bootloader_cpu_info_t structure to receive the CPU information.
 * @return true if the CPU information was successfully retrieved, false if the index was out of bounds.
 */
bool bootloader_get_cpu(size_t index, bootloader_cpu_info_t* info);

/**
 * @brief Gets the module information for the module at the specified index.
 * @param index The index of the module to get information for. Must be less than g_bootloader_info.module_count.
 * @param module A pointer to a bootloader_module_t structure to receive the module information.
 * @return true if the module information was successfully retrieved, false if the index was out of bounds.
 */
bool bootloader_get_module(size_t index, bootloader_module_t* module);

/**
 * @brief Starts the AP at the specified index with the specified argument.
 * @param index The index of the AP to start. Must be less than g_bootloader_info.cpu_count and must refer to an AP that can be booted (see bootloader_cpu_info_t.can_boot).
 * @param arg The argument to pass to the AP. This argument is always the kernels core id for that ap
 * @return true if the AP was successfully started, false if the index was out of bounds or if the AP could not be started for some reason (e.g. the AP is not present or does not support being booted by the bootloader).
 */
bool bootloader_start_ap(size_t index, uint64_t arg);

/**
 * @brief Maps the kernel segments provided by the bootloader into the kernel's address space. This should only be called once during early initialization before paging is fully set up
 */
void bootloader_map_kernel_segments();

/**
 * @brief Updates the address of the framebuffer at the specified index. This should be called by the kernel whenever it remaps the framebuffer to a new address to ensure that the bootloader can still access the framebuffer.
 * @param index The index of the framebuffer to update the address for. Must be less than g_bootloader_info.framebuffer_count.
 * @param address The new address of the framebuffer.
 */
void bootloader_set_framebuffer_address(size_t i, void* address);

/**
 * @brief Converts a bootloader memory map entry type to a string representation.
 * @param type The bootloader memory map entry type to convert.
 * @return A string representation of the bootloader memory map entry type.
 */
const char* bootloader_memmap_type_to_str(bootloader_mmap_entry_type_t type);

/**
 * @brief Converts a bootloader type to a string representation.
 * @param type The bootloader type to convert.
 * @return A string representation of the bootloader type.
 */
const char* bootloader_type_to_str(bootloader_type_t type);
