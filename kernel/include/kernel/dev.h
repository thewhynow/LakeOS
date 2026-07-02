#include <kernel/vfs.h>

#ifndef __DEV_H
#define __DEV_H

typedef struct t_DeviceFile t_DeviceFile;

struct t_DeviceFile {
    t_FileChrono modified;

    char *name;

    size_t (*f_read)
        (void *buff, size_t bytes);
    size_t (*f_write)
        (const void *buff, size_t bytes);

    t_DeviceFile *next;
};

extern t_VFSOperations dev_vfs_ops;

t_FSContext DEV_init();

void DEV_uninit();

t_FSNode DEV_root();

t_DeviceFile *DEV_lookup(t_FSNode parent, const char *name);

size_t DEV_read(t_FSNode *file, size_t bytes, void *buff);

size_t DEV_write(t_FSNode *file, size_t bytes, const void *buff);

t_DeviceFile *DEV_readdir(t_FSNode dir, size_t n);

const char *DEV_nodename(t_FSNode *file);

void DEV_fstat(t_FSNode *file, t_FileStat *out);

#endif
