#include <common/tty.h>
#include <fs/vfs.h>
#include <memory/heap.h>
#include <string.h>

// @todo: this whole "filesystem driver" is fucking laughable

#define NODES(VFS) ((devfs_nodes_t*) (VFS)->private_data)

typedef struct {
    vfs_node_t* root;
    vfs_node_t* tty;
} devfs_nodes_t;

static vfs_node_ops_t g_tty_ops;

static vfs_result_t devfs_tty_node_attr(vfs_node_t* node, vfs_node_attr_t* attr) {
    (void) node;
    attr->size = 0;
    attr->type = node->type;
    attr->permissions = VFS_MODE_PERM_USER_R | VFS_MODE_PERM_USER_W | VFS_MODE_PERM_GROUP_R | VFS_MODE_PERM_GROUP_W | VFS_MODE_PERM_OTHER_R | VFS_MODE_PERM_OTHER_W;
    return VFS_RESULT_OK;
}

static vfs_result_t devfs_tty_node_read(vfs_node_t* node, void* buf, size_t size, size_t offset, size_t* read_count) {
    (void) node;
    (void) buf;
    (void) size;
    (void) offset;
    (void) read_count;

    if(g_tty) {
        char* data = tty_read(g_tty, read_count);
        size_t to_copy = *read_count < size ? *read_count : size;
        memcpy(buf, data, to_copy);
        heap_free(data, *read_count);
        *read_count = to_copy;
        return VFS_RESULT_OK;
    }

    assert(false);
    return VFS_RESULT_ERR_UNSUPPORTED;
}

static vfs_result_t tty_node_write(vfs_node_t* node, const void* buf, size_t size, size_t offset, size_t* write_count) {
    (void) node;
    (void) offset;
    nl_printf("%.*s", (int) size, (char*) buf);
    *write_count = size;
    return VFS_RESULT_OK;
}

static vfs_result_t devfs_tty_node_lookup(vfs_node_t* node, const char* name, vfs_node_t** out) {
    (void) node;
    (void) name;
    (void) out;
    return VFS_RESULT_ERR_NOT_DIR;
}

static vfs_result_t devfs_tty_node_readdir(vfs_node_t* node, size_t* offset, char** out) {
    (void) node;
    (void) offset;
    (void) out;
    return VFS_RESULT_ERR_NOT_DIR;
}


static vfs_node_ops_t g_tty_ops = {
    .attr = devfs_tty_node_attr,
    .lookup = devfs_tty_node_lookup,
    .read = devfs_tty_node_read,
    .write = tty_node_write,
    .readdir = devfs_tty_node_readdir,
};

static vfs_result_t devfs_root_node_attr(vfs_node_t* node, vfs_node_attr_t* attr) {
    (void) node;
    attr->size = 0;
    attr->type = VFS_NODE_TYPE_DIR;
    return VFS_RESULT_OK;
}

static vfs_result_t devfs_root_node_lookup(vfs_node_t* node, const char* name, vfs_node_t** out) {
    if(strcmp(name, "..") == 0) {
        *out = node->parent;
        return 0;
    }
    if(strcmp(name, ".") == 0) {
        *out = node;
        return 0;
    }
    if(strcmp(name, "tty") == 0) {
        *out = NODES(node->vfs)->tty;
        return 0;
    }
    return VFS_RESULT_ERR_NOT_FOUND;
}

static vfs_result_t devfs_root_node_read(vfs_node_t* node, void* buf, size_t size, size_t offset, size_t* read_count) {
    (void) node;
    (void) buf;
    (void) size;
    (void) offset;
    (void) read_count;
    return VFS_RESULT_ERR_NOT_FILE;
}

static vfs_result_t devfs_root_node_write(vfs_node_t* node, const void* buf, size_t size, size_t offset, size_t* written_count) {
    (void) node;
    (void) buf;
    (void) size;
    (void) offset;
    (void) written_count;
    return VFS_RESULT_ERR_NOT_FILE;
}

static vfs_result_t devfs_root_node_readdir(vfs_node_t* node, size_t* offset, char** out) {
    (void) node;
    switch(*offset) {
        case 0:  *out = "tty"; break;
        default: *out = nullptr; return 0;
    }
    (*offset)++;
    return 0;
}

static vfs_node_ops_t g_root_ops = {
    .attr = devfs_root_node_attr,
    .lookup = devfs_root_node_lookup,
    .read = devfs_root_node_read,
    .write = devfs_root_node_write,
    .readdir = devfs_root_node_readdir,
};

static vfs_result_t devfs_mount(vfs_t* vfs) {
    devfs_nodes_t* nodes = heap_alloc(sizeof(devfs_nodes_t));

    nodes->root = heap_alloc(sizeof(vfs_node_t));
    memset(nodes->root, 0, sizeof(vfs_node_t));
    nodes->root->vfs = vfs;
    nodes->root->type = VFS_NODE_TYPE_DIR;
    nodes->root->ops = &g_root_ops;
    nodes->root->parent = vfs->mount_point;

    // just a little fake tty node bash please fucking work
    nodes->tty = heap_alloc(sizeof(vfs_node_t));
    memset(nodes->tty, 0, sizeof(vfs_node_t));
    nodes->tty->vfs = vfs;
    nodes->tty->type = VFS_NODE_TYPE_CHARDEV;
    nodes->tty->ops = &g_tty_ops;
    nodes->tty->parent = nodes->root;

    vfs->private_data = (void*) nodes;
    return 0;
}

static vfs_result_t devfs_unmount(vfs_t* vfs) {
    devfs_nodes_t* nodes = NODES(vfs);
    heap_free(nodes->root, sizeof(vfs_node_t));
    heap_free(nodes->tty, sizeof(vfs_node_t));
    heap_free(nodes, sizeof(devfs_nodes_t));
    return 0;
}

static vfs_result_t devfs_get_root_node(vfs_t* vfs, vfs_node_t** out) {
    *out = NODES(vfs)->root;
    return 0;
}

const vfs_ops_t g_vfs_devfs_ops = { .mount = devfs_mount, .unmount = devfs_unmount, .get_root_node = devfs_get_root_node };
