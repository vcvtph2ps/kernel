#pragma once
#include <common/userspace/process.h>
#include <fs/vfs.h>
#include <memory/memory.h>
#include <memory/vm.h>

typedef struct {
    // @note: address of where we should start execution
    virt_addr_t executable_entry_point;
    // @note: address of the entry point in the image
    virt_addr_t image_entry_point;

    virt_addr_t phdr_table;
    size_t phnum;
    size_t phentsize;

    // @note: offset of the image in memory from it's lowest address (for DYN images)
    virt_addr_t image_offset;

    // @note: load base of the interpreter (ld.so); 0 if no interpreter (AT_BASE)
    virt_addr_t interp_base;
} elfldr_elf_loader_info_t;

bool elfldr_load_file(vm_address_space_t* address_space, vfs_path_t* path, elfldr_elf_loader_info_t** loader_info);
