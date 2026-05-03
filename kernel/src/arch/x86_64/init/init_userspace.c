#include "init_stages.h"
#include <common/tty.h>
#include <common/userspace/syscall.h>
#include <common/sched/sched.h>
#include <common/boot/bootloader.h>
#include <common/arch.h>
#include <fs/vfs.h>
#include <memory/heap.h>
#include <memory/ptm.h>
#include <memory/vm.h>
#include <memory/memory.h>
#include <ldr/elfldr.h>
#include <ldr/sysv.h>
#include <common/userspace/process.h>
#include <lib/log.h>

void fn_init_stage_arch_userspace(uint32_t core_id) {
    (void) core_id;
    g_tty = tty_init();
    syscall_init();
    sched_init_bsp();
}

void fn_init_stage_vfs(uint32_t core_id) {
    (void) core_id;
    for(size_t i = 0; i < g_bootloader_info.module_count; i++) {
        bootloader_module_t module = {};
        if(!bootloader_get_module(i, &module)) {
            // @todo: fixme assert(false)
            continue;
        }
        LOG_INFO("Module %zu: %s, address: %p, size: %zu\n", i, module.path, module.address, module.size);
    }

    bootloader_module_t initramfs_module;
    bool found_initramfs = bootloader_find_module("/boot/initramfs.rdk", &initramfs_module);
    if(!found_initramfs) { arch_panic("Failed to find initramfs\n"); }

    vfs_result_t res = vfs_mount(&g_vfs_rdsk_ops, nullptr, (void*) initramfs_module.address);
    if(res != VFS_RESULT_OK) { arch_panic("Failed to mount initramfs (%d)\n", res); }
    LOG_OKAY("mounted initramfs\n");

    vfs_node_t* root_node;
    res = vfs_root(&root_node);
    if(res != VFS_RESULT_OK) { arch_panic("Failed to get root node (%d)\n", res); }

    res = vfs_mount(&g_vfs_devfs_ops, &VFS_MAKE_ABS_PATH("/dev"), nullptr);
    if(res != VFS_RESULT_OK) { arch_panic("Failed to mount devfs (%d)\n", res); }
}

void fn_init_stage_userspace(uint32_t core_id) {
    (void) core_id;
    vm_address_space_t* process_as = heap_alloc(sizeof(vm_address_space_t));
    ptm_init_user(process_as);

    elfldr_elf_loader_info_t* elf_info;
    bool loaded_elf = elfldr_load_file(process_as, &VFS_MAKE_ABS_PATH("/usr/bin/bash"), &elf_info);
    if (!loaded_elf) { arch_panic("Failed to load init file"); }

    g_tty = tty_init();

    size_t stack_virt_size = 1024 * PAGE_SIZE_DEFAULT;
    virt_addr_t user_stack = (virt_addr_t) vm_map_anon(process_as, (void*) (MEMORY_USERSPACE_END - (10 * PAGE_SIZE_DEFAULT) - stack_virt_size), stack_virt_size, VM_PROT_RW, VM_CACHE_NORMAL, VM_FLAG_FIXED | VM_FLAG_ZERO | VM_FLAG_DYNAMICALLY_BACKED);
    virt_addr_t user_rsp = sysv_user_stack_init(process_as, user_stack + stack_virt_size, elf_info);
    process_t* process = process_create(process_as, elf_info->executable_entry_point, user_rsp);

    // @todo: what the fuck
    sched_thread_schedule(CONTAINER_OF(process->threads.head, thread_t, list_node_proc));
}
