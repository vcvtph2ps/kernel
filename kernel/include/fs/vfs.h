#pragma once

#include <list.h>

typedef struct vfs vfs_t;
typedef struct vfs_node vfs_node_t;
typedef struct vfs_ops vfs_ops_t;
typedef struct vfs_node_ops vfs_node_ops_t;

typedef enum {
    VFS_RESULT_OK = 0,
    VFS_RESULT_ERR_UNSUPPORTED,
    VFS_RESULT_ERR_NOT_FOUND,
    VFS_RESULT_ERR_NOT_FILE,
    VFS_RESULT_ERR_NOT_DIR,
    VFS_RESULT_ERR_EXISTS,
    VFS_RESULT_ERR_READ_ONLY,
    VFS_RESULT_ERR_BUFFER_TOO_SMALL
} vfs_result_t;

typedef enum {
    VFS_NODE_TYPE_FILE,
    VFS_NODE_TYPE_DIR,
    VFS_NODE_TYPE_CHARDEV
} vfs_node_type_t;

typedef struct {
    size_t size;
    vfs_node_type_t type;
    uint64_t permissions;
} vfs_node_attr_t;

struct vfs_ops {
    /**
     * @brief Mounts the filesystem represented by this vfs at the specified mount point
     * @note This function should only be called once per vfs instance, and the vfs should not be used for any other operations until it is successfully mounted
     */
    vfs_result_t (*mount)(vfs_t* vfs);

    /**
     * @brief Unmounts the filesystem represented by this vfs from its mount point
     * @note This function should only be called on the vfs that is currently mounted at a mount point
     */
    vfs_result_t (*unmount)(vfs_t* vfs);

    /**
     * @brief Gets the root node of the filesystem represented by this vfs
     * @param out_root A pointer to a vfs_node_t* variable to receive the root node pointer
     * @return VFS_RESULT_OK if the root node was successfully retrieved, or an appropriate error code otherwise
     */
    vfs_result_t (*get_root_node)(vfs_t* vfs, vfs_node_t** out_root);
};

struct vfs_node_ops {
    /**
     * @brief Looks up a child node with the specified name under the given node Only valid for directory nodes
     * @param node The directory node to look under
     * @param name The name of the child node to look up
     * @param out_node A pointer to a vfs_node_t* variable to receive the child node pointer if the lookup is successful
     * @return VFS_RESULT_OK if the child node was successfully found and retrieved, or an appropriate error code otherwise
     */
    vfs_result_t (*lookup)(vfs_node_t* node, const char* name, vfs_node_t** out_node);

    /**
     * @brief Reads data from the given file node into the provided buffer
     * @param node The file node to read from
     * @param buffer The buffer to read the data into
     * @param size The maximum number of bytes to read into the buffer
     * @param offset The byte offset within the file to start reading from
     * @param out_bytes_read A pointer to a size_t variable to receive the actual number of bytes read into the buffer
     * @return VFS_RESULT_OK if the read operation was successful, or an appropriate error code otherwise
     */
    vfs_result_t (*read)(vfs_node_t* node, void* buffer, size_t size, size_t offset, size_t* out_bytes_read);

    /**
     * @brief Writes data from the provided buffer into the given file node
     * @param node The file node to write to
     * @param buffer The buffer containing the data to write
     * @param size The number of bytes to write from the buffer into the file
     * @param offset The byte offset within the file to start writing to
     * @param out_bytes_written A pointer to a size_t variable to receive the actual number of bytes written into the file
     * @return VFS_RESULT_OK if the write operation was successful, or an appropriate error code otherwise
     */
    vfs_result_t (*write)(vfs_node_t* node, const void* buffer, size_t size, size_t offset, size_t* out_bytes_written);

    /**
     * @brief Reads the names of child nodes under the given directory node into the provided buffer, starting from the specified offset
     * @param node The directory node to read from
     * @param offset The index of the child node to start reading from (e.g. 0 for the first child, 1 for the second child, etc.)
     * @param name_buffer The buffer to read the child node name into
     * @param name_buffer_size The size of the name_buffer in bytes
     * @param out_bytes_read A pointer to a size_t variable to receive the actual number of bytes read into the name_buffer
     * @return VFS_RESULT_OK if the readdir operation was successful, or an appropriate error code otherwise
     */
    vfs_result_t (*readdir)(vfs_node_t* node, size_t* offset, char** name);

    /**
     * @brief Gets the attributes of the given node
     * @param node The node to get the attributes of
     * @param out_attr A pointer to a vfs_node_attr_t variable to receive the node attributes
     * @return VFS_RESULT_OK if the attributes were successfully retrieved, or an appropriate error code otherwise
     */
    vfs_result_t (*attr)(vfs_node_t* node, vfs_node_attr_t* out_attr);
};

struct vfs {
    const vfs_ops_t* ops;
    void* private_data;

    vfs_node_t* mount_point;
    list_node_t vfs_list_node;
};

struct vfs_node {
    vfs_t* vfs;
    vfs_t* mounted_vfs;
    vfs_node_t* parent;
    vfs_node_ops_t* ops;
    vfs_node_type_t type;
    void* private_data;
};

typedef struct {
    vfs_node_t* node;
    const char* rel_path;
} vfs_path_t;

#define VFS_MAKE_ABS_PATH(__path)               \
    (vfs_path_t) {                              \
        .node = (nullptr), .rel_path = (__path) \
    }
#define VFS_MAKE_REL_PATH(__node, __path)      \
    (vfs_path_t) {                             \
        .node = (__node), .rel_path = (__path) \
    }

#define VFS_MODE_PERM_USER_RWX 0700
#define VFS_MODE_PERM_USER_R 0400
#define VFS_MODE_PERM_USER_W 0200
#define VFS_MODE_PERM_USER_X 0100
#define VFS_MODE_PERM_GROUP_RWX 070
#define VFS_MODE_PERM_GROUP_R 040
#define VFS_MODE_PERM_GROUP_W 020
#define VFS_MODE_PERM_GROUP_X 010
#define VFS_MODE_PERM_OTHER_RWX 07
#define VFS_MODE_PERM_OTHER_R 04
#define VFS_MODE_PERM_OTHER_W 02
#define VFS_MODE_PERM_OTHER_X 01

extern list_t g_vfs_list;

/**
 * @brief Create and mount a new vfs instance from ops at path
 * @param ops The vfs_ops_t structure containing the function pointers for the vfs operations to use for this vfs instance
 * @param path The path to mount the vfs at
 * @param private_data A pointer to private data to associate with this vfs instance
 * @return VFS_RESULT_OK if the vfs was successfully created and mounted, or an appropriate error code otherwise
 */
vfs_result_t vfs_mount(const vfs_ops_t* ops, const vfs_path_t* path, void* private_data);

/**
 * @brief Unmounts the vfs that is currently mounted at the specified path
 * @param path The path to unmount the vfs from
 * @return VFS_RESULT_OK if the vfs was successfully unmounted, or an appropriate error code otherwise
 */
vfs_result_t vfs_unmount(const char* path);

/**
 * @brief Gets the root node of the filesystem mounted at the specified path
 * @param root_node A pointer to a vfs_node_t* variable to receive the root node pointer
 * @return VFS_RESULT_OK if the root node was successfully retrieved, or an appropriate error code otherwise
 */
vfs_result_t vfs_root(vfs_node_t** root_node);

/**
 * @brief Looks up the node at the specified path and returns a pointer to it in result_node
 * @param path The path to look up the node at.
 * @param result_node A pointer to a vfs_node_t* variable to receive the node pointer if the lookup is successful
 * @return VFS_RESULT_OK if the node was successfully found and retrieved, or an appropriate
 */
vfs_result_t vfs_lookup(const vfs_path_t* path, vfs_node_t** result_node);

/**
 * @brief Reads data from the file at the specified path into the provided buffer
 * @param path The path to the file to read from
 * @param buf The buffer to read the data into
 * @param size The maximum number of bytes to read into the buffer
 * @param offset The byte offset within the file to start reading from
 * @param read_count A pointer to a size_t variable to receive the actual number of bytes read into the buffer
 * @return VFS_RESULT_OK if the read operation was successful, or an appropriate error code otherwise
 */
vfs_result_t vfs_read(vfs_path_t* path, void* buf, size_t size, size_t offset, size_t* read_count);

/**
 * @brief Writes data from the provided buffer into the file at the specified path
 * @param path The path to the file to write to
 * @param buf The buffer containing the data to write
 * @param size The number of bytes to write from the buffer into the file
 * @param offset The byte offset within the file to start writing to
 * @param written_count A pointer to a size_t variable to receive the actual number of bytes written into the file
 * @return VFS_RESULT_OK if the write operation was successful, or an appropriate error code otherwise
 */
vfs_result_t vfs_write(vfs_path_t* path, const void* buf, size_t size, size_t offset, size_t* written_count);

/**
 * @brief Gets the attributes of the given node
 * @param path The path to the node to get the attributes of
 * @param attr A pointer to a vfs_node_attr_t variable to receive the node attributes
 * @return VFS_RESULT_OK if the attributes were successfully retrieved, or an appropriate error code otherwise
 */
vfs_result_t vfs_attr(vfs_path_t* path, vfs_node_attr_t* attr);

/**
 * @brief Constructs a path string representing the absolute path to the given node and writes it into out_buf
 * @param path The path to the directory node
 * @param out_buf A pointer to a char* variable to receive the buffer containing the path string. The caller is responsible for freeing this buffer after use.
 * @param out_size A pointer to a size_t variable to receive the size of the path string in bytes, including the null terminator
 * @return VFS_RESULT_OK if the path string was successfully constructed and written into out_buf, or an appropriate error code otherwise
 */
vfs_result_t vfs_path_to(vfs_node_t* node, char** out_buf, size_t* out_size);

extern const vfs_ops_t g_vfs_rdsk_ops;
