#include <kernel/kmm.h>
#include <kernel/rtc.h>
#include <kernel/vfm.h>
#include <kernel/dev.h>
#include <string.h>
#include <stdio.h>

#define _VFS_H_INTERNAL
#include <kernel/vfs.h>
#undef _VFS_H_INTERNAL

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

void VFS_init_virt(){
    t_FSNode virt_root = VFM_root();
    t_VFSNode *virt_root_vnode = VFS_make_vnode(&vfs_root, virt_root);
    virt_root_vnode->driver = &vfm_vfs_ops;
    VFS_insert_vnode(&vfs_root, virt_root_vnode);
}

void VFS_init_dev(){
    t_FSNode dev_root = DEV_root();
    t_VFSNode *dev_root_vnode = VFS_make_vnode(&vfs_root, dev_root);
    dev_root_vnode->driver = &dev_vfs_ops;
    VFS_insert_vnode(&vfs_root, dev_root_vnode);

    t_VFSNode *vfs_stdin = VFS_lookup(dev_root_vnode, "STDIN"),
             *vfs_stdout = VFS_lookup(dev_root_vnode, "STDOUT")
    ;
    
    t_FileDescriptor *fd0 = kmalloc(sizeof *fd0),
                     *fd1 = kmalloc(sizeof *fd1)
    ;

    *fd0 = (t_FileDescriptor){
        .descriptor = vfs_stdin->handle,
        .driver     = vfs_stdin->driver
    };

    *fd1 = (t_FileDescriptor){
        .descriptor = vfs_stdout->handle,
        .driver     = vfs_stdout->driver
    };

    VFS_add_descriptor(fd0);
    VFS_add_descriptor(fd1);
}

void VFS_init_rootfs(){
    if (!VFS_lookup(&vfs_root, "LAKEOS")){
        printf(
            "This disk does not have LakeOS installed.\n"
            "Do you want to install? y/n\n"
        );
        char ans = getchar();

        if (ans == 'y')
            printf("Installing...\n");
        else
            printf("Fuck you. Installing...\n");

        /* marker file for LakeOS */
        vfs_root.driver->f_Create(vfs_root.handle, "LAKEOS", FILE_ATTRIB_SYSTEM);
        vfs_root.driver->f_Create(vfs_root.handle, "USER", FILE_ATTRIB_DIRECTORY);
        vfs_root.driver->f_Create(vfs_root.handle, "BIN",  FILE_ATTRIB_DIRECTORY);
    }

    VFS_init_virt();
    VFS_init_dev();
}

void VFS_init(){
    uint32_t num;
    storage_device_t *dev = SAL_get_devices(&num);

retry_mounting:
    for (uint32_t i = 0; i < num; ++i){
        t_VFSOperations *rootfsops;
        t_FSContext *ctx = VFS_try_mount(dev + i, &rootfsops);

        if (ctx){
        reread_input:
            printf("Mount root on on %s? y/n\n", dev[i].name);
            char ans = getchar();

            if (ans == 'y'){
                VFS_mount_root(ctx, rootfsops); 
                break;
            }
            else if (ans == 'n')
                continue;
            else {
                printf("Please enter either 'y' or 'n'\n");
                goto reread_input;
            }
        }
    }

    if (!vfs_root.driver){
        printf("No boot device selected. Retry...\n");
        goto retry_mounting;
    }

    VFS_init_rootfs();
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

void VFS_insert_vnode(t_VFSNode *parent, t_VFSNode *child){
    t_VFSNode *iter = parent->children;

    if (!iter){
        parent->children = child;
        return;
    }

    while (iter->next)
        iter = iter->next;

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

    kfree(path);

    return iter;
}

t_VFSNode *VFS_get_dir_and_fname(const char *_path, char **out_fname){
    char *path = strdup(_path);

    int i = strlen(path) - 1;

    for (; i >= 0; --i)
        if (path[i] == '/'){
            path[i] = '\0';
            break;
        }

    *out_fname = (char*)_path + i + 1;

    t_VFSNode *dir = VFS_walk_path(path);

    kfree(path);

    return dir;
}

size_t VFS_make_file
    (t_VFSNode *parent, const char *name, uint8_t attribs)
{
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
        VFS_insert_vnode(parent, vnode);
        return vnode;
    }

    return NULL;
}

static t_FSFile *descriptor_list;
static size_t descriptor_list_size;

int VFS_add_descriptor(t_FSFile file){
    for (size_t i = 0; i < descriptor_list_size; ++i){
        if (descriptor_list[i] == NULL){
            descriptor_list[i] = file;
            return i;
        }
    }

    /* if the list needs to be expanded */

    descriptor_list = kexpand(descriptor_list, sizeof file);
    descriptor_list[descriptor_list_size] = file;
    return descriptor_list_size++;
}

void VFS_rem_descriptor(int fd){
    /* lazily deallocate */
    descriptor_list[fd] = NULL;
}

/**
 * final vfs goddamn api
 */

int VFS_open(const char *path, uint8_t mode){
    t_VFSNode *vnode = VFS_walk_path(path);

    t_FSFile file = vnode->driver->f_Open(vnode->handle, mode);

    t_FileDescriptor *res = kmalloc(sizeof(t_FileDescriptor));

    *res = (t_FileDescriptor){
        .descriptor = file,
        .driver     = vnode->driver
    };

    return VFS_add_descriptor(res);
}

void VFS_close(int fd){
    t_FileDescriptor *descriptor = descriptor_list[fd];
    descriptor->driver->f_Close(descriptor->descriptor);
    kfree(descriptor);
}

size_t VFS_write(int fd, void *data, size_t len){
    t_FileDescriptor *descriptor = descriptor_list[fd];

    return
        descriptor->driver->f_Write(descriptor->descriptor, len, data);
}

size_t VFS_read(int fd, void *data, size_t len){
    t_FileDescriptor *descriptor = descriptor_list[fd];

    return
        descriptor->driver->f_Read(descriptor->descriptor, len, data);
}

void VFS_create(const char *path, uint8_t attributes){
    char *file_name;
    t_VFSNode *dir_vnode = VFS_get_dir_and_fname(path, &file_name);

    VFS_make_file(dir_vnode, file_name, attributes);
}

void VFS_remove(const char *path){
    t_VFSNode *vnode = VFS_walk_path(path);

    vnode->driver->f_Remove(vnode->handle);

    t_VFSNode *iter = vnode->parent->children;

    while (iter->next != vnode)
        iter = iter->next;

    iter->next = iter->next->next;

    kfree(vnode->handle);
}

void VFS_stat(const char *path, const t_FileStat *stat){
    t_VFSNode *vnode = VFS_walk_path(path);
    vnode->driver->f_Stat(vnode->handle, stat);
}
