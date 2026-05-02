#include <arch/cpu_local.h>
#include <common/userspace/syscall.h>
#include <fs/vfs.h>
#include <log.h>
#include <string.h>

#include "memory/heap.h"

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

syscall_ret_t syscall_sys_open(syscall_args_t args) {
    virt_addr_t pathname_str = args.arg1;
    virt_addr_t pathname_len = args.arg2;
    size_t flags = args.arg3;
    size_t mode = args.arg4;

    (void) flags; // @todo:
    (void) mode; // @todo:

    if(pathname_len == 0 || pathname_len > 256) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_INVAL); }
    if(!userspace_validate_buffer(CPU_LOCAL_GET_CURRENT_THREAD()->common.process, pathname_str, pathname_len)) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT); }

    char pathname[256];
    memcpy(pathname, (const void*) pathname_str, pathname_len);
    pathname[pathname_len] = '\0';

    LOG_STRC("syscall_sys_open: pid=%lu, pathname=%s\n", CPU_LOCAL_GET_CURRENT_THREAD()->common.process->pid, pathname);

    vfs_node_t* node;
    if(vfs_lookup(&VFS_MAKE_ABS_PATH(pathname), &node) != VFS_RESULT_OK) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_NOENT); }

    fd_store_t* store = CPU_LOCAL_GET_CURRENT_THREAD()->common.process->fd_store;
    fd_data_t* fd_data = (fd_data_t*) heap_alloc(sizeof(fd_data_t));
    if(!fd_data) {
        LOG_WARN("syscall_sys_open: Failed to allocate fd_data for open syscall\n");
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_NOMEM);
    }
    fd_data->node = node;
    fd_data->cursor = 0;
    int fd = fd_store_allocate(store, fd_data);
    if(fd == -1) {
        LOG_WARN("syscall_sys_open: Failed to allocate fd for open syscall\n");
        heap_free(fd_data, sizeof(fd_data_t));
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_NOMEM);
    }

    LOG_STRC("syscall_sys_open: fd=%lu\n", fd);
    return SYSCALL_RET_VALUE(fd);
}


syscall_ret_t syscall_sys_read(syscall_args_t args) {
    uint64_t fd = args.arg1;
    virt_addr_t buf = args.arg2;
    size_t count = args.arg3;
    LOG_STRC("syscall_sys_read: pid=%lu, fd=%d, count=%lu\n", CPU_LOCAL_GET_CURRENT_THREAD()->common.process->pid, fd, count);

    if(count == 0) { return SYSCALL_RET_VALUE(0); }
    if(!userspace_validate_buffer(CPU_LOCAL_GET_CURRENT_THREAD()->common.process, buf, count)) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT); }

    fd_store_t* store = CPU_LOCAL_GET_CURRENT_THREAD()->common.process->fd_store;
    fd_data_t* node = fd_store_get_fd(store, fd);
    if(!node) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD); }
    if(!node->node) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD); }
    if(node->node->type == VFS_NODE_TYPE_DIR) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD); }

    // @hack: yield might clear uap flag. so we need to do some dumbass shit
    void* kernel_buffer = heap_alloc(count);
    if(kernel_buffer == nullptr) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_NOMEM); }
    size_t read_count = 0;
    vfs_result_t result = node->node->ops->read(node->node, kernel_buffer, count, node->cursor, &read_count);

    vm_copy_to(CPU_LOCAL_GET_CURRENT_THREAD()->common.process->address_space, buf, kernel_buffer, read_count);
    if(result != VFS_RESULT_OK) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD); }

    node->cursor += read_count;
    return SYSCALL_RET_VALUE(read_count);
}

syscall_ret_t syscall_sys_write(syscall_args_t args) {
    uint64_t fd = args.arg1;
    virt_addr_t buf = args.arg2;
    size_t count = args.arg3;
    LOG_STRC("syscall_sys_write: pid=%lu, fd=%d, count=%lu\n", CPU_LOCAL_GET_CURRENT_THREAD()->common.process->pid, fd, count);

    if(count == 0) { return SYSCALL_RET_VALUE(0); }
    if(!userspace_validate_buffer(CPU_LOCAL_GET_CURRENT_THREAD()->common.process, buf, count)) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT); }

    fd_store_t* store = CPU_LOCAL_GET_CURRENT_THREAD()->common.process->fd_store;
    fd_data_t* node = fd_store_get_fd(store, fd);
    if(!node) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD); }
    if(!node->node) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD); }
    if(node->node->type == VFS_NODE_TYPE_DIR) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD); }

    // @hack: yield might clear uap flag. so we need to do some dumbass shit
    void* kernel_buffer = heap_alloc(count);
    vm_copy_from(kernel_buffer, CPU_LOCAL_GET_CURRENT_THREAD()->common.process->address_space, buf, count);

    // @note: hackkkkk
    if(fd == 1 || fd == 2) {
        nl_printf("%.*s", count, kernel_buffer);
        heap_free(kernel_buffer, count);
        return SYSCALL_RET_VALUE(count);
    }

    size_t write_count = 0;
    vfs_result_t result = node->node->ops->write(node->node, kernel_buffer, count, node->cursor, &write_count);

    heap_free(kernel_buffer, count);

    if(result == VFS_RESULT_ERR_READ_ONLY) return SYSCALL_RET_ERROR(SYSCALL_ERROR_ROFS);
    if(result != VFS_RESULT_OK) return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);

    node->cursor += write_count;
    return SYSCALL_RET_VALUE(write_count);
}

syscall_ret_t syscall_sys_close(uint64_t fd) {
    LOG_STRC("syscall_sys_close: pid=%lu, fd=%d\n", CPU_LOCAL_GET_CURRENT_THREAD()->common.process->pid, fd);
    fd_store_t* store = CPU_LOCAL_GET_CURRENT_THREAD()->common.process->fd_store;
    if(!fd_store_close(store, fd)) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD); }

    return SYSCALL_RET_VALUE(0);
}

syscall_ret_t syscall_sys_seek(syscall_args_t args) {
    uint64_t fd = args.arg1;
    size_t offset = args.arg2;
    size_t whence = args.arg3;
    LOG_STRC("syscall_sys_seek: pid=%lu, fd=%d, offset=%ld, whence=%d\n", CPU_LOCAL_GET_CURRENT_THREAD()->common.process->pid, fd, offset, whence);

    fd_store_t* store = CPU_LOCAL_GET_CURRENT_THREAD()->common.process->fd_store;
    fd_data_t* node = fd_store_get_fd(store, fd);
    if(!node) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD); }
    if(!node->node) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD); }
    if(node->node->type == VFS_NODE_TYPE_DIR) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD); }
    if(node->node->type == VFS_NODE_TYPE_CHARDEV) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_SPIPE); }
    int64_t signed_offset = (int64_t) offset;
    vfs_node_attr_t node_attr;
    if(node->node->ops->attr(node->node, &node_attr) != VFS_RESULT_OK) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_SPIPE); }

    int64_t signed_file_size = (int64_t) node_attr.size;

    int64_t new_cursor = 0;
    if(whence == SEEK_SET) {
        new_cursor = signed_offset;
    } else if(whence == SEEK_CUR) {
        new_cursor = node->cursor + signed_offset;
    } else if(whence == SEEK_END) {
        new_cursor = signed_file_size + signed_offset;
    } else {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_INVAL);
    }

    LOG_INFO("new_cursor=%ld, file size=%ld\n", new_cursor, signed_file_size);
    if(new_cursor < 0 || new_cursor > signed_file_size) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_RANGE); }
    node->cursor = new_cursor;

    return SYSCALL_RET_VALUE(0);
}

syscall_ret_t syscall_sys_is_a_tty(syscall_args_t args) {
    uint64_t fd = args.arg1;
    // @todo: STUB
    fd_store_t* store = CPU_LOCAL_GET_CURRENT_THREAD()->common.process->fd_store;
    fd_data_t* node = fd_store_get_fd(store, fd);
    if(!node) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD); }
    if(!node->node) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD); }
    if(node->node->type == VFS_NODE_TYPE_DIR) { return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD); }
    // @todo: NOT ALL CHAR DEVS ARE TTYS
    if(node->node->type == VFS_NODE_TYPE_CHARDEV) { return SYSCALL_RET_VALUE(0); }

    return SYSCALL_RET_ERROR(SYSCALL_ERROR_NOTTY);
}
