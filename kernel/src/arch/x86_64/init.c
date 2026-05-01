#include <arch/hardware/fpu.h>
#include <arch/hardware/lapic.h>
#include <arch/internal/cpuid.h>
#include <common/arch.h>
#include <common/boot/bootloader.h>
#include <common/cpu_local.h>
#include <common/interrupts/dw.h>
#include <common/interrupts/interrupt.h>
#include <common/sched/sched.h>
#include <common/userspace/process.h>
#include <common/userspace/syscall.h>
#include <fs/vfs.h>
#include <lib/log.h>
#include <memory/heap.h>
#include <memory/memory.h>
#include <memory/pmm.h>
#include <memory/ptm.h>
#include <memory/slab.h>
#include <memory/vm.h>

static uint32_t g_arch_ap_finished = 0;

void init_aps() {
    uint32_t current_core_id = 1;

    for(size_t i = 0; i < g_bootloader_info.cpu_count; i++) {
        bootloader_cpu_info_t cpu_info;
        if(!bootloader_get_cpu(i, &cpu_info)) { arch_panic("Failed to get cpu info for cpu %zu\n", i); }
        if(cpu_info.is_bsp) { continue; }
        if(!cpu_info.can_boot) { continue; }
        ATOMIC_STORE(&g_arch_ap_finished, 0, ATOMIC_RELAXED);
        LOG_OKAY("Starting AP with core id %u\n", current_core_id);
        bootloader_start_ap(i, current_core_id);
        while(ATOMIC_LOAD(&g_arch_ap_finished, ATOMIC_ACQUIRE) == 0) { arch_relax(); }
        current_core_id++;
    }
}

void mount_initramfs() {
    for(size_t i = 0; i < g_bootloader_info.module_count; i++) {
        bootloader_module_t module = {};
        if(!bootloader_get_module(i, &module)) {
            assert(false);
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

    size_t offset = 0;
    while(1) {
        char* dirent_name;
        res = root_node->ops->readdir(root_node, &offset, &dirent_name);
        if(res == VFS_RESULT_ERR_NOT_FOUND) { break; }
        assertf(res == VFS_RESULT_OK, "Failed to readdir %d", res);
        if(dirent_name == nullptr) { break; }

        vfs_node_t* dirent;
        res = root_node->ops->lookup(root_node, dirent_name, &dirent);
        assertf(res == VFS_RESULT_OK, "Failed to lookup dirent %d", res);

        vfs_node_attr_t attr;
        res = dirent->ops->attr(dirent, &attr);
        assertf(res == VFS_RESULT_OK, "Failed to get dirent attr %d", res);
        LOG_INFO("%s: %s %d bytes\n", dirent->type == VFS_NODE_TYPE_DIR ? "dir" : "file", dirent_name, attr.size);
    }
}

void arch_init_bsp() {
    cpu_local_init_bsp();
    pmm_init();
    ptm_init_kernel_bsp();
    log_init();

    slab_cache_init();
    heap_init();

    LOG_INFO("CPU Vendor: %s\n", arch_cpuid_get_vendor_string());
    LOG_INFO("CPU Name: %s\n", arch_cpuid_get_name_string());

    arch_gdt_init_common();
    LOG_OKAY("GDT INIT OKAY!\n");
    dw_init_bsp();
    interrupt_init_bsp();
    LOG_OKAY("INTERRUPT INIT OKAY!\n");
    arch_lapic_init_bsp();
    LOG_OKAY("LAPIC INIT OKAY!\n");

    arch_fpu_init_bsp();
    LOG_OKAY("FPU INIT OKAY!\n");

    cpu_local_init_storage(g_bootloader_info.cpu_count);
    init_aps();
    syscall_init();
    sched_init_bsp();

    uint8_t process_bytes[] = { 0x48, 0xc7, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x48, 0xc7, 0xc7, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x05 };

    vm_address_space_t* process_as = heap_alloc(sizeof(vm_address_space_t));
    ptm_init_user(process_as);

    size_t stack_virt_size = 1024 * PAGE_SIZE_DEFAULT;
    virt_addr_t user_stack = (virt_addr_t) vm_map_anon(process_as, (void*) (MEMORY_USERSPACE_END - (10 * PAGE_SIZE_DEFAULT) - stack_virt_size), stack_virt_size, VM_PROT_RW, VM_CACHE_NORMAL, VM_FLAG_FIXED | VM_FLAG_ZERO | VM_FLAG_DYNAMICALLY_BACKED);
    virt_addr_t user_stack_top = user_stack + stack_virt_size;
    void* code = vm_map_anon(process_as, VM_NO_HINT, ALIGN_UP(sizeof(process_bytes) / sizeof(uint8_t), PAGE_SIZE_DEFAULT), VM_PROT_RWX, VM_CACHE_NORMAL, VM_FLAG_NONE);
    process_t* process = process_create(process_as, (virt_addr_t) code, user_stack_top);

    vm_copy_to(process_as, (uintptr_t) code, process_bytes, sizeof(process_bytes));

    // @todo: what the fuck
    sched_thread_schedule(CONTAINER_OF(process->threads.head, thread_t, list_node_proc));

    mount_initramfs();

    char buf[1024];
    size_t out;
    vfs_read(&VFS_MAKE_ABS_PATH("hello.txt"), buf, 1024, 0, &out);
    LOG_INFO("read %zu bytes from hello.txt: %.*s\n", out, (int) out, buf);

    sched_arch_handoff();
    while(1);
}

void arch_init_ap(uint32_t core_id) {
    (void) core_id;
    ptm_init_kernel_ap();

    LOG_OKAY("core %u early init\n", core_id);

    arch_lapic_init_ap_early();
    cpu_local_init_ap(core_id);
    arch_gdt_init_common();
    interrupt_init_ap();
    arch_fpu_init_ap();

    ATOMIC_STORE(&g_arch_ap_finished, 1, ATOMIC_RELAXED);

    while(1);
}
