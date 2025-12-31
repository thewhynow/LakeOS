#ifndef _VFS_H
#define _VFS_H

#include "sal.h"

typedef void 
    /**
     * t_FSContext represents a
     *  filesystem context, 
     *  starting from the root
     *  directory
     */
    *t_FSContext,
    /**
     * t_FSNode represents an
     *  abstract handle that
     *  should be used to 
     *  represent a file to
     *  the FS. It is always
     *  allocated using kmalloc
     *  and friends, and is
     *  the responsibility of
     *  the VFS to free the handle
     * If a file that a handle
     *  points to is deleted,
     *  behavior of using a
     *  handle afterwards is
     *  undefined
     */
    *t_FSNode,
    /**
     * t_FSFile represents an
     *  open context of a file,
     *  and multiple should be
     *  able to exist
     *  corresponding to the
     *  same file in one scope.
     *  When the file is closed,
     *  the file context is 
     *  also freed as well.
     */
    *t_FSFile;

typedef struct {
    uint16_t year;
    uint8_t  seconds,
             minutes,
             /* 24-hour format */
             hours,
             day_of_month,
             month_of_year;
} t_FileChrono;

typedef enum {
    FILE_ATTRIB_DIRECTORY = 1,
    FILE_ATTRIB_READ_ONLY,
    FILE_ATTRIB_HIDDEN,
    FILE_ATTRIB_SYSTEM
} e_FileAttribs;

typedef struct {
    t_FileChrono created,
                 modified;
    size_t  size,
            flags;
} t_FileStat;

typedef struct {
    /**
     * mounts the fs on the specified storage
     *  device and initializes all relevant
     *  structures
     * returns NULL on failure
     */
    t_FSContext (*f_Mount)(storage_device_t *dev);
    /**
     * unmounts the specified fs context
     *  and frees all relevant allocated
     *  memory, INCLUDING all t_FSNode's
     */
    void        (*f_UnMount)(t_FSContext);
    /**
     * returns a t_FSNode which points
     *  to the corresponding root dir
     *  on the filesystem
     */
    t_FSNode    (*f_Root)(t_FSContext);
    /**
     * returns a t_FSNode which points
     *  to a file named <name> in the
     *  directory pointer to by <node>
     */
    t_FSNode    (*f_Lookup)(t_FSNode, const char *);
    /**
     * creates a file named <name> in
     *  the directory pointed to by
     *  <node> */
    void        (*f_Create)(t_FSNode, const char *, uint8_t);
    /**
     * removes a file pointed to by
     *  <node> from the fs 
     * DOES NOT handle recursively 
     *  deleting directories and their
     *  files
     */
    void        (*f_Remove)(t_FSNode);
    /**
     * opens a file pointed to by
     *  <node> with <mode> passed
     *  onto the fs, and returns a
     *  t_FSFile handle which can be
     *  used to read and write to the
     *  file
     */
    t_FSFile    (*f_Open)(t_FSNode, uint8_t);
    /**
     * closes a file pointed to by
     *  <file> and updates its data
     *  in the fs
     */
    void        (*f_Close)(t_FSFile);
    /**
     * read n bytes from <file> into <buff>
     */
    size_t      (*f_Read)(t_FSFile, size_t, void *);
    /**
     * write n bytes from <buff> into <file>
     */
    size_t      (*f_Write)(t_FSFile, size_t, const void *);
    /**
     * return n file from directory pointed
     *  to by <node>, return NULL on end-of
     *  -directory
     */
    t_FSNode    (*f_ReadDir)(t_FSNode, size_t);
    /**
     * returns the name of a file pointed
     *  to by <node>
     */
    const char *(*f_NodeName)(t_FSNode);
    /**
     * copies the relevant t_FileStat fields
     *  for the specified file node
     */
    void        (*f_Stat)(t_FSNode, t_FileStat *);

    /**
     * the fs driver is not responsible for
     *  managing memory of the handles returned,
     *  that is the responsibility of the VFS.
     *  the FS driver should be written to ensure
     *  that the VFS is able to free a handle
     *  arbitrarily (obviously not before passing
     *  it as a parameter)
     *
     * this implies that the VFS is repsonsible for
     *  maintaining a tree of cached file handles
     *  and even vnodes, and is also responsible
     *  for freeing them.
     */
} t_VFSOperations;

void VFS_init();

void VFS_register_fs(t_VFSOperations *ops);

void VFS_curr_chrono(t_FileChrono *out);


#endif

#ifdef _VFS_H_INTERNAL

typedef struct {
    const char *mountpoint;
    const t_VFSOperations *ops;
    const t_FSContext *ctx;
} t_MountPoint;

typedef struct t_VFSNode {
    struct t_VFSNode *parent, 
    /* linked list of children */
                     *children,
                     *next;
    t_FSNode *handle;
    t_VFSOperations *driver; 
} t_VFSNode;

size_t VFS_create(t_VFSNode *parent, const char *name, uint8_t attribs);
t_VFSNode *VFS_lookup(t_VFSNode *parent, const char *name);
t_VFSNode *VFS_walk_path(const char *_path);
#endif
