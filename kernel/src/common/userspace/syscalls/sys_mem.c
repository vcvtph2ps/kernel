#include <arch/cpu_local.h>
#include <assert.h>
#include <common/userspace/syscall.h>
#include <log.h>

#define PROT_NONE 0x00
#define PROT_READ 0x01
#define PROT_WRITE 0x02
#define PROT_EXEC 0x04

#define MAP_FILE 0x00
#define MAP_SHARED 0x01
#define MAP_PRIVATE 0x02
#define MAP_FIXED 0x10
#define MAP_ANON 0x20

syscall_ret_t syscall_sys_vm_map(syscall_args_t args) {
    virt_addr_t hint = args.arg1;
    size_t size = args.arg2;
    size_t prot = args.arg3;
    size_t flags = args.arg4;
    size_t fd = args.arg5;
    size_t offset = args.arg6;

    LOG_STRC("hint=%p size=%zu prot=0x%lx flags=0x%lx fd=%d offset=0x%lx\n", hint, size, prot, flags, fd, offset);
    assert((flags & (MAP_ANON)) != 0);

    vm_protection_t vm_prot = VM_PROT_RO;
    if(prot & PROT_READ) {
        vm_prot.read = true;
    }
    if(prot & PROT_WRITE) {
        vm_prot.write = true;
    }
    if(prot & PROT_EXEC) {
        vm_prot.execute = true;
    }

    uint64_t vm_flags = VM_FLAG_NONE;
    if(flags & MAP_FIXED) {
        vm_flags |= VM_FLAG_FIXED;
    }

    process_t* current_process = CPU_LOCAL_GET_CURRENT_THREAD()->common.process;

    virt_addr_t vaddr = (virt_addr_t) vm_map_anon(current_process->address_space, (void*) hint, ALIGN_UP(size, PAGE_SIZE_DEFAULT), vm_prot, VM_CACHE_NORMAL, vm_flags);
    if(vaddr == 0) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_INVAL);
    }
    return SYSCALL_RET_VALUE(vaddr);
}

syscall_ret_t syscall_sys_vm_unmap(syscall_args_t args) {
    void* addr = (void*) args.arg1;
    size_t size = args.arg2;

    process_t* current_process = CPU_LOCAL_GET_CURRENT_THREAD()->common.process;
    vm_unmap(current_process->address_space, addr, size);
    return SYSCALL_RET_VALUE(0);
}

syscall_ret_t syscall_sys_vm_protect(syscall_args_t args) {
    void* addr = (void*) args.arg1;
    size_t size = args.arg2;
    size_t prot = args.arg3;

    vm_protection_t vm_prot = VM_PROT_RO;
    if(prot & PROT_READ) {
        vm_prot.read = true;
    }
    if(prot & PROT_WRITE) {
        vm_prot.write = true;
    }
    if(prot & PROT_EXEC) {
        vm_prot.execute = true;
    }

    process_t* current_process = CPU_LOCAL_GET_CURRENT_THREAD()->common.process;

    LOG_STRC("addr=%p size=%zu prot=%zu\n", addr, size, prot);

    vm_rewrite_prot(current_process->address_space, (void*) addr, size, vm_prot);
    return SYSCALL_RET_VALUE(0);
}
