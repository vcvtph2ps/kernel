#include <arch/cpu_local.h>
#include <common/userspace/structs.h>
#include <common/userspace/syscall.h>
#include <fs/vfs.h>
#include <log.h>
#include <memory/heap.h>
#include <string.h>

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

    if(pathname_len == 0 || pathname_len > 256) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_INVAL);
    }
    if(!userspace_validate_buffer(CPU_LOCAL_GET_CURRENT_THREAD()->common.process, pathname_str, pathname_len)) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT);
    }

    char pathname[256];
    memcpy(pathname, (const void*) pathname_str, pathname_len);
    pathname[pathname_len] = '\0';

    LOG_STRC("pid=%lu, pathname=%s\n", CPU_LOCAL_GET_CURRENT_THREAD()->common.process->pid, pathname);

    vfs_node_t* node;
    if(vfs_lookup(&VFS_MAKE_REL_PATH(CPU_LOCAL_GET_CURRENT_THREAD()->common.process->cwd, pathname), &node) != VFS_RESULT_OK) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_NOENT);
    }

    fd_store_t* store = CPU_LOCAL_GET_CURRENT_THREAD()->common.process->fd_store;
    fd_data_t* fd_data = (fd_data_t*) heap_alloc(sizeof(fd_data_t));
    if(!fd_data) {
        LOG_WARN("Failed to allocate fd_data for open syscall\n");
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_NOMEM);
    }
    fd_data->node = node;
    fd_data->cursor = 0;
    int fd = fd_store_allocate(store, fd_data);
    if(fd == -1) {
        LOG_WARN("Failed to allocate fd for open syscall\n");
        heap_free(fd_data, sizeof(fd_data_t));
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_NOMEM);
    }

    LOG_STRC("fd=%lu\n", fd);
    return SYSCALL_RET_VALUE(fd);
}


syscall_ret_t syscall_sys_read(syscall_args_t args) {
    uint64_t fd = args.arg1;
    virt_addr_t buf = args.arg2;
    size_t count = args.arg3;
    LOG_STRC("pid=%lu, fd=%d, count=%lu\n", CPU_LOCAL_GET_CURRENT_THREAD()->common.process->pid, fd, count);

    if(count == 0) {
        return SYSCALL_RET_VALUE(0);
    }
    if(!userspace_validate_buffer(CPU_LOCAL_GET_CURRENT_THREAD()->common.process, buf, count)) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT);
    }

    fd_store_t* store = CPU_LOCAL_GET_CURRENT_THREAD()->common.process->fd_store;
    fd_data_t* node = fd_store_get_fd(store, fd);
    if(!node) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }
    if(!node->node) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }
    if(node->node->type == VFS_NODE_TYPE_DIR) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }

    // @hack: yield might clear uap flag. so we need to do some dumbass shit
    void* kernel_buffer = heap_alloc(count);
    if(kernel_buffer == nullptr) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_NOMEM);
    }
    size_t read_count = 0;
    vfs_result_t result = node->node->ops->read(node->node, kernel_buffer, count, node->cursor, &read_count);

    vm_copy_to(CPU_LOCAL_GET_CURRENT_THREAD()->common.process->address_space, buf, kernel_buffer, read_count);
    heap_free(kernel_buffer, count);

    if(result != VFS_RESULT_OK) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }

    node->cursor += read_count;
    return SYSCALL_RET_VALUE(read_count);
}

syscall_ret_t syscall_sys_write(syscall_args_t args) {
    uint64_t fd = args.arg1;
    virt_addr_t buf = args.arg2;
    size_t count = args.arg3;
    LOG_STRC("pid=%lu, fd=%d, count=%lu\n", CPU_LOCAL_GET_CURRENT_THREAD()->common.process->pid, fd, count);

    if(count == 0) {
        return SYSCALL_RET_VALUE(0);
    }
    if(!userspace_validate_buffer(CPU_LOCAL_GET_CURRENT_THREAD()->common.process, buf, count)) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT);
    }

    fd_store_t* store = CPU_LOCAL_GET_CURRENT_THREAD()->common.process->fd_store;
    fd_data_t* node = fd_store_get_fd(store, fd);
    if(!node) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }
    if(!node->node) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }
    if(node->node->type == VFS_NODE_TYPE_DIR) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }

    // @hack: yield might clear uap flag. so we need to do some dumbass shit
    void* kernel_buffer = heap_alloc(count);
    vm_copy_from(kernel_buffer, CPU_LOCAL_GET_CURRENT_THREAD()->common.process->address_space, buf, count);

    size_t write_count = 0;
    vfs_result_t result = node->node->ops->write(node->node, kernel_buffer, count, node->cursor, &write_count);

    heap_free(kernel_buffer, count);

    if(result == VFS_RESULT_ERR_READ_ONLY) return SYSCALL_RET_ERROR(SYSCALL_ERROR_ROFS);
    if(result != VFS_RESULT_OK) return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);

    node->cursor += write_count;
    return SYSCALL_RET_VALUE(write_count);
}

syscall_ret_t syscall_sys_close(syscall_args_t args) {
    uint64_t fd = args.arg1;
    LOG_STRC("pid=%lu, fd=%d\n", CPU_LOCAL_GET_CURRENT_THREAD()->common.process->pid, fd);
    fd_store_t* store = CPU_LOCAL_GET_CURRENT_THREAD()->common.process->fd_store;
    if(!fd_store_close(store, fd)) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }

    return SYSCALL_RET_VALUE(0);
}

syscall_ret_t syscall_sys_seek(syscall_args_t args) {
    uint64_t fd = args.arg1;
    size_t offset = args.arg2;
    size_t whence = args.arg3;
    LOG_STRC("pid=%lu, fd=%d, offset=%ld, whence=%d\n", CPU_LOCAL_GET_CURRENT_THREAD()->common.process->pid, fd, offset, whence);

    fd_store_t* store = CPU_LOCAL_GET_CURRENT_THREAD()->common.process->fd_store;
    fd_data_t* node = fd_store_get_fd(store, fd);
    if(!node) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }
    if(!node->node) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }
    if(node->node->type == VFS_NODE_TYPE_DIR) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }
    if(node->node->type == VFS_NODE_TYPE_CHARDEV) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_SPIPE);
    }
    int64_t signed_offset = (int64_t) offset;
    vfs_node_attr_t node_attr;
    if(node->node->ops->attr(node->node, &node_attr) != VFS_RESULT_OK) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_SPIPE);
    }

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

    LOG_STRC("new_cursor=%ld, file size=%ld\n", new_cursor, signed_file_size);
    if(new_cursor < 0 || new_cursor > signed_file_size) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_RANGE);
    }
    node->cursor = new_cursor;

    return SYSCALL_RET_VALUE(0);
}

syscall_ret_t syscall_sys_is_a_tty(syscall_args_t args) {
    uint64_t fd = args.arg1;
    LOG_STRC("pid=%lu, fd=%d\n", CPU_LOCAL_GET_CURRENT_THREAD()->common.process->pid, fd);

    // @todo: STUB
    fd_store_t* store = CPU_LOCAL_GET_CURRENT_THREAD()->common.process->fd_store;
    fd_data_t* node = fd_store_get_fd(store, fd);
    if(!node) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }
    if(!node->node) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }
    if(node->node->type == VFS_NODE_TYPE_DIR) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }
    // @todo: NOT ALL CHAR DEVS ARE TTYS
    if(node->node->type == VFS_NODE_TYPE_CHARDEV) {
        return SYSCALL_RET_VALUE(0);
    }

    return SYSCALL_RET_ERROR(SYSCALL_ERROR_NOTTY);
}

syscall_ret_t syscall_sys_get_cwd(syscall_args_t args) {
    virt_addr_t buf = args.arg1;
    size_t size = args.arg2;

    LOG_STRC("buf=%p, size=%lu\n", buf, size);
    process_t* process = CPU_LOCAL_GET_CURRENT_THREAD()->common.process;

    char* kernel_buf;
    size_t kernel_buf_size;
    vfs_result_t res = vfs_path_to(process->cwd, &kernel_buf, &kernel_buf_size);
    if(res != VFS_RESULT_OK) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT);
    }

    size_t cwd_len = strlen(kernel_buf) + 1;
    if(cwd_len > size) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_RANGE);
    }

    vm_copy_to(process->address_space, buf, kernel_buf, cwd_len);

    heap_free(kernel_buf, kernel_buf_size);
    return SYSCALL_RET_VALUE(0);
}

syscall_ret_t stat_internal(structs_stat_t* statbuf, vfs_node_t* vfs_node) {
    vfs_node_attr_t attr;
    vfs_result_t res = vfs_node->ops->attr(vfs_node, &attr);
    if(res != VFS_RESULT_OK) {
        assert(false);
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT);
    }

    memset(statbuf, 0, sizeof(structs_stat_t));

    statbuf->st_size = attr.size;
    statbuf->st_nlink = 1; // @todo:
    statbuf->st_mode = 0;
    switch(attr.type) {
        case VFS_NODE_TYPE_FILE:    statbuf->st_mode |= STRUCTS_STAT_MODE_TYPE_FILE; break;
        case VFS_NODE_TYPE_DIR:     statbuf->st_mode |= STRUCTS_STAT_MODE_TYPE_DIR; break;
        case VFS_NODE_TYPE_CHARDEV: statbuf->st_mode |= STRUCTS_STAT_MODE_TYPE_CHAR; break;
        default:                    assert(false); return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT);
    }

    statbuf->st_mode |= attr.permissions;
    return SYSCALL_RET_VALUE(0);
}

#define AT_FDCWD -100

syscall_ret_t syscall_sys_stat(syscall_args_t args) {
    uint64_t fd = args.arg1;
    virt_addr_t statbuf = args.arg2;

    LOG_STRC("pid=%lu, fd=%d\n", CPU_LOCAL_GET_CURRENT_THREAD()->common.process->pid, fd);
    if(!userspace_validate_buffer(CPU_LOCAL_GET_CURRENT_THREAD()->common.process, statbuf, sizeof(structs_stat_t))) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT);
    }

    fd_store_t* store = CPU_LOCAL_GET_CURRENT_THREAD()->common.process->fd_store;
    fd_data_t* node = fd_store_get_fd(store, fd);
    if(!node) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }
    if(!node->node) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
    }

    structs_stat_t buf;
    syscall_ret_t ret = stat_internal(&buf, node->node);
    if(ret.is_error) {
        return ret;
    }
    vm_copy_to(CPU_LOCAL_GET_CURRENT_THREAD()->common.process->address_space, statbuf, &buf, sizeof(structs_stat_t));
    return ret;
}

syscall_ret_t syscall_sys_stat_at(syscall_args_t args) {
    uint64_t fd = args.arg1;
    uint64_t path = args.arg2;
    size_t path_len = args.arg3;
    uint64_t statbuf = args.arg4;
    size_t flag = args.arg5;

    (void) flag;
    if(!userspace_validate_buffer(CPU_LOCAL_GET_CURRENT_THREAD()->common.process, statbuf, sizeof(structs_stat_t))) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT);
    }

    if(path_len == 0 || path_len > 256) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_INVAL);
    }
    if(!userspace_validate_buffer(CPU_LOCAL_GET_CURRENT_THREAD()->common.process, path, path_len)) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_FAULT);
    }

    char pathname[256];
    vm_copy_from(pathname, CPU_LOCAL_GET_CURRENT_THREAD()->common.process->address_space, path, path_len);
    pathname[path_len] = '\0';
    process_t* process = CPU_LOCAL_GET_CURRENT_THREAD()->common.process;
    LOG_STRC("pid=%lu, fd=%d, path=%s\n", process->pid, fd, pathname);

    vfs_path_t vfs_path;

    if(pathname[0] == '/') {
        vfs_path.node = nullptr;
        vfs_path.rel_path = pathname;
    } else {
        if(((int32_t) fd) == AT_FDCWD) {
            vfs_path.node = process->cwd;
        } else {
            fd_store_t* store = process->fd_store;
            fd_data_t* node = fd_store_get_fd(store, fd);
            if(!node) {
                return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
            }
            if(!node->node) {
                return SYSCALL_RET_ERROR(SYSCALL_ERROR_BADFD);
            }
            vfs_path.node = node->node;
        }
        vfs_path.rel_path = pathname;
    }

    vfs_node_t* result_node;
    vfs_result_t res = vfs_lookup(&vfs_path, &result_node);
    if(res != VFS_RESULT_OK) {
        return SYSCALL_RET_ERROR(SYSCALL_ERROR_NOENT);
    }

    structs_stat_t buf;
    syscall_ret_t ret = stat_internal(&buf, result_node);
    if(ret.is_error) {
        return ret;
    }
    vm_copy_to(CPU_LOCAL_GET_CURRENT_THREAD()->common.process->address_space, statbuf, &buf, sizeof(structs_stat_t));
    return ret;
}
