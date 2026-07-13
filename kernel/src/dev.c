#include <kernel/dev.h>
#include <kernel/tty.h>
#include <kernel/kmm.h>
#include <kernel/ps2.h>

t_VFSOperations dev_vfs_ops = (t_VFSOperations){
    .f_Mount    = (void*) DEV_init,
    .f_UnMount  = (void*) DEV_uninit,
    .f_Root     = (void*) DEV_root,
    .f_Lookup   = (void*) DEV_lookup,
    .f_Create   = (void*) NULL,
    .f_Remove   = (void*) NULL,
    .f_Open     = (void*) NULL,
    .f_Close    = (void*) NULL,
    .f_Read     = (void*) DEV_read,
    .f_Write    = (void*) DEV_write,
    .f_ReadDir  = (void*) DEV_readdir,
    .f_NodeName = (void*) DEV_nodename,
    .f_Stat     = (void*) DEV_fstat,
    .f_Seek     = (void*) NULL,
};

static t_DeviceFile *device_files;
static t_FileChrono dev_files_init_chrono;

t_DeviceFile *DEV_add_file(t_DeviceFile *file){
    t_DeviceFile *list = device_files;

    while (list->next)
        list = list->next;

    list->next = kmalloc(sizeof *file);
    *list->next = *file;

    list->next->next = NULL;

    return list->next;
}

void DEV_remove_file(t_DeviceFile *file){
    t_DeviceFile *iter = device_files;

    while (iter->next != file)
        iter = iter->next;

    iter->next = file->next;

    kfree(file->name);
    kfree(file);
}

/**
 * VFS API START
 */

t_FSContext DEV_init(){
    VFS_curr_chrono(&dev_files_init_chrono);

    device_files = kmalloc(sizeof(t_DeviceFile));

    t_DeviceFile stdout = (t_DeviceFile){
        .name = strdup("STDOUT"),
        .f_write = (void*) terminal_write,
        .f_read  = NULL,
        .next = NULL
    };

    *device_files = stdout;

    t_DeviceFile stdin = (t_DeviceFile){
        .name = strdup("STDIN"),
        .f_read = PS2_read_bytes,
        .f_write = NULL
    };

    DEV_add_file(&stdin);

    return (void*) &dev_vfs_ops;
}

void DEV_uninit(){
    t_DeviceFile *iter = device_files;

    while (iter){
        kfree(iter->name);
        t_DeviceFile *next = iter->next;
        kfree(iter);
        iter = next;
    }
}

t_FSNode DEV_root(){
    /* reference it so it doesn't collide w/ STDOUT */
    return &device_files;
}

t_DeviceFile *DEV_lookup(t_FSNode parent, const char *name){
    if (*(t_DeviceFile**)parent != device_files)
        return NULL;

    for (
        t_DeviceFile *iter = device_files;
        iter != NULL;
        iter = iter->next
    )
        if (!strcmp(iter->name, name))
            return iter;

    return NULL;
}

size_t DEV_read(t_FSNode *file, size_t bytes, void *buff){
    t_DeviceFile *descriptor = (t_DeviceFile*) file;

    return descriptor->f_read(buff, bytes);
}

size_t DEV_write(t_FSNode *file, size_t bytes, const void *buff){
    t_DeviceFile *descriptor = (t_DeviceFile*) file;

    return descriptor->f_write(buff, bytes);
}

t_DeviceFile *DEV_readdir(t_FSNode dir, size_t n){
    t_DeviceFile *iter = device_files;
    size_t i = 0;

    while (iter->next && i < n){
        iter = iter->next;
        ++i;
    }

    if (i != n)
        return NULL;
    else
        return iter;
}

const char *DEV_nodename(t_FSNode *file){
    if (*(t_DeviceFile**)file == device_files)
        return "DEV";
    else
        return ((t_DeviceFile*) file)->name;
}

void DEV_fstat(t_FSNode *file, t_FileStat *out){
    if (*(t_DeviceFile**)file == device_files){
        *out = (t_FileStat){0};
        return;
    }

    t_DeviceFile *descriptor = (t_DeviceFile*) file;

    *out = (t_FileStat){
        .created  = dev_files_init_chrono,
        .modified = descriptor->modified,
        .flags = FILE_ATTRIB_SYSTEM,
        .size = 0
    };
}

