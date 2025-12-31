#define _VFS_H_INTERNAL
#include "../include/vfs.h"
#include "../include/kmm.h"
#include "../include/rtc.h"
#include "../../libc/include/stdio.h"
#include "../../libc/include/string.h"

/**
 * a vector of all filesystem drivers
 *  registered to the VFS
 */
static t_VFSOperations *vfsops;
static size_t num_drivers;

static t_MountPoint *mounts;
static size_t num_mounts;
static t_VFSNode vfs_root;

void VFS_register_fs(t_VFSOperations *driver){
    vfsops = kexpand(vfsops, sizeof *driver);
    vfsops[num_drivers++] = *driver;
}

t_FSContext *VFS_try_mount(storage_device_t *dev, t_VFSOperations **out){
    t_FSContext *ctx;
    for (size_t i = 0; i < num_drivers; ++i){
        ctx = vfsops[i].f_Mount(dev); 
        *out = vfsops;
        if (ctx) return ctx;
    }
    return NULL;
}

void VFS_mount_root(t_FSContext *fs, t_VFSOperations *fsops){
    vfs_root = (t_VFSNode){
        .parent = NULL, .children = NULL, .next = NULL,
        .driver = fsops,
        .handle = fsops->f_Root(fs),
    };
}

void VFS_init_rootfs(){
    vfs_root.driver->f_Create(vfs_root.handle, "USER",      FILE_ATTRIB_DIRECTORY);
    vfs_root.driver->f_Create(vfs_root.handle, "BIN",       FILE_ATTRIB_DIRECTORY);
    vfs_root.driver->f_Create(vfs_root.handle, "SYS",       FILE_ATTRIB_DIRECTORY);
    vfs_root.driver->f_Create(vfs_root.handle, "TEST.TXT",  0);
}

void VFS_init(){
    uint32_t num;
    storage_device_t *dev = SAL_get_devices(&num);
    bool root_mounted;
    for (uint32_t i = 0; i < num; ++i){
        t_VFSOperations *rootfsops;
        t_FSContext *ctx = VFS_try_mount(dev + i, &rootfsops);
        if (ctx){
            VFS_mount_root(ctx, rootfsops);
            break; 
        }
    }

    VFS_init_rootfs();
    VFS_walk_path("/USER/TWHYN/HELLO.TXT");
}

void VFS_curr_chrono(t_FileChrono *out){
    *out = (t_FileChrono){
        .seconds = time.year,
        .minutes = time.minutes,
        .hours   = time.hours,
        .day_of_month = time.monthday,
        .month_of_year = time.month,
        .year = time.year
   };
}

void VFS_insert_node(t_VFSNode *parent, t_VFSNode *child){
    t_VFSNode *iter = parent->children;
    if (!iter) parent->children = child;

    while (!iter->next) iter = iter->next;
    iter->next = child;
}

t_VFSNode *VFS_make_vnode(t_VFSNode *parent, t_FSNode *handle){
    t_VFSNode *node = kmalloc(sizeof(t_VFSNode));
    *node = (t_VFSNode){
        .parent = parent,
        .children = NULL,
        .next = NULL,
        .handle = handle,
        .driver = parent->driver
    };
    return node;
}

t_VFSNode *VFS_walk_path(const char *_path){
    char *path = strdup(_path);
    char *name = strtok(path, "/");
    t_VFSNode *iter = &vfs_root;

    for (; name; name = strtok(NULL, "/")){
        iter = VFS_lookup(iter, name);
        if (!iter) return iter;
    }

    return iter;
}

/* VFS API */

size_t VFS_create(t_VFSNode *parent, const char *name, uint8_t attribs){
    t_VFSNode *node = VFS_lookup(parent, name); 
    if (node) return 1;
    parent->driver->f_Create(parent->handle, name, attribs);
    return 0;
}

t_VFSNode *VFS_lookup(t_VFSNode *parent, const char *name){
    for (t_VFSNode *node = parent->children; node; node = node->next){
        const char *nodename = node->driver->f_NodeName(node->handle);
        if (!strcmp(nodename, name)) return node;
    }
    
    t_FSNode *node = parent->driver->f_Lookup(parent->handle, name);
    if (node){
        t_VFSNode *vnode = VFS_make_vnode(parent, node);
        VFS_insert_node(parent, vnode);
        return vnode;
     }

    return NULL;
}


