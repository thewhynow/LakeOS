/**
 * @file vfs.h
 * @brief virtual filesystem, the one api for every filesystem driver
 *  (fat, vfm, dev). paths start at the root fs and drivers are
 *  attached as vnodes. the user only ever sees file descriptors
 */

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

/**
 * a date and time, the year is the full year
 */
typedef struct {
    uint16_t year;
    uint8_t  seconds,       ///< 0-59
             minutes,       ///< 0-59
             /** 24-hour format */
             hours,
             day_of_month,  ///< 1-31
             month_of_year; ///< 1-12
} t_FileChrono;

/**
 * file attributes, or'd together
 */
typedef enum {
    /** the file is a directory */
    FILE_ATTRIB_DIRECTORY = 1,
    /** writes should fail */
    FILE_ATTRIB_READ_ONLY = 2,
    /** doesn't show up in normal listings */
    FILE_ATTRIB_HIDDEN    = 4,
    /** belongs to the operating system */
    FILE_ATTRIB_SYSTEM    = 8
} e_FileAttribs;

/**
 * what stat gives back about a file
 */
typedef struct {
    t_FileChrono created,
                 modified;
    /** in bytes */
    size_t  size,
    /** e_FileAttribs or'd together */
            flags;
} t_FileStat;

/**
 * the table of functions a filesystem driver gives the VFS
 *  a driver leaves the ones it can't do NULL (dev has no create, for example)
 *
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
     *  to a file named @p name in the
     *  directory pointer to by @p node
     * the arguments are the directory node and the name
     */
    t_FSNode    (*f_Lookup)(t_FSNode, const char *);
    /**
     * creates a file named @p name in
     *  the directory pointed to by
     *  @p node
     * the arguments are the directory node, the name,
     *  and the e_FileAttribs of the new file
     */
    void        (*f_Create)(t_FSNode, const char *, uint8_t);
    /**
     * removes a file pointed to by
     *  @p node from the fs
     * DOES NOT handle recursively
     *  deleting directories and their
     *  files
     */
    void        (*f_Remove)(t_FSNode);
    /**
     * opens a file pointed to by
     *  @p node with @p mode passed
     *  onto the fs, and returns a
     *  t_FSFile handle which can be
     *  used to read and write to the
     *  file
     * the arguments are the node and the mode,
     *  e_VFSFILEMODES or'd together
     */
    t_FSFile    (*f_Open)(t_FSNode, uint8_t);
    /**
     * closes a file pointed to by
     *  @p file and updates its data
     *  in the fs
     */
    void        (*f_Close)(t_FSFile);
    /**
     * read n bytes from @p file into @p buff
     * the arguments are the file, n, and the buffer,
     *  returns how many bytes were read
     */
    size_t      (*f_Read)(t_FSFile, size_t, void *);
    /**
     * write n bytes from @p buff into @p file
     * the arguments are the file, n, and the buffer,
     *  returns how many bytes were written
     */
    size_t      (*f_Write)(t_FSFile, size_t, const void *);
    /**
     * return n file from directory pointed
     *  to by @p node, return NULL on end-of
     *  -directory
     * the arguments are the directory node and n
     */
    t_FSNode    (*f_ReadDir)(t_FSNode, size_t);
    /**
     * returns the name of a file pointed
     *  to by @p node
     */
    const char *(*f_NodeName)(t_FSNode);
    /**
     * copies the relevant t_FileStat fields
     *  for the specified file node
     * the arguments are the node and where to copy to
     */
    void        (*f_Stat)(t_FSNode, t_FileStat *);

    /**
     * sets the file position to the value provided
     *  assumes that the poisition is within the bounds
     *   of the file
     *  if the position is set to (-1) then the function
     *   returns the current position of the file, else
     *   the function returns the position of the file
     *   after the seek
     * the arguments are the file and the position
     */
    size_t      (*f_Seek)(t_FSFile, size_t);
} t_VFSOperations;

/**
 * @brief mounts the root filesystem: tries every registered driver
 *  on every storage device and asks (y/n) which one to use, then
 *  mounts the virtual fs and the device fs under it and opens
 *  stdin (fd 0) and stdout (fd 1)
 *  asks to "install" if the disk has no LAKEOS marker file
 */
void VFS_init();

/**
 * @brief adds a filesystem driver, the table is copied
 * @param ops the driver
 */
void VFS_register_fs(t_VFSOperations *ops);

/**
 * @brief gets the current time from the rtc
 * @param out where the time goes
 */
void VFS_curr_chrono(t_FileChrono *out);

/**
 * @brief puts an open file in the descriptor table, in the first empty slot
 * @param file the open file
 * @return the file descriptor
 */
int VFS_add_descriptor(t_FSFile file);

/**
 * @brief empties a slot of the descriptor table, doesn't close or free anything
 * @param fd the file descriptor
 */
void VFS_rem_descriptor(int fd);

/**
 * VFS PUBLIC API START
 */

/**
 * @brief opens a file
 * @param path full path from the root, '/' seperated
 * @param mode e_VFSFILEMODES or'd together
 * @return the file descriptor
 */
int VFS_open(const char *path, uint8_t mode);

/**
 * @brief closes a file and frees its descriptor
 * @param descriptor the file descriptor
 */
void VFS_close(int descriptor);

/**
 * @brief writes to an open file
 * @param descriptor the file descriptor
 * @param data the data
 * @param len how many bytes to write
 * @return how many bytes were written
 */
size_t VFS_write(int descriptor, void *data, size_t len);

/**
 * @brief reads from an open file
 * @param descriptor the file descriptor
 * @param data where the data goes
 * @param len how many bytes to read
 * @return how many bytes were read
 */
size_t VFS_read(int descriptor, void *data, size_t len);

/**
 * @brief creates a file or directory, the filesystem of the
 *  directory it's in is used
 * @param path full path of the new file
 * @param attributes e_FileAttribs or'd together
 */
void VFS_create(const char *path, uint8_t attributes);

/**
 * @brief removes a file
 * @param path full path of the file
 */
void VFS_remove(const char *path);

/**
 * @brief gets info about a file by path
 * @param path full path of the file
 * @param stat where the info goes
 */
void VFS_stat(const char *path, const t_FileStat *stat);

/**
 * @brief gets info about an open file
 * @param descriptor the file descriptor
 * @param stat where the info goes
 */
void VFS_fstat(int descriptor, const t_FileStat *stat);

/**
 * @brief moves the position of an open file. past the end it
 *  fills the file with zeros up to the new position
 * @param descriptor the file descriptor
 * @param offset how far to move, relative to whence
 * @param whence an e_VFSSEEKMODES
 * @return the new position, -1 on failure
 */
size_t VFS_seek(int descriptor, ssize_t offset, int whence);

/**
 * modes used for opening files
 */

typedef enum {
    VFS_FILE_READ  = 0b00000001,
    VFS_FILE_WRITE = 0b00000010,
    VFS_FILE_APPND = 0b00000100,
    VFS_FILE_EOF   = 0b00001000
} e_VFSFILEMODES;

/**
 * where VFS_seek counts the offset from
 */
typedef enum {
    /** the start of the file */
    VFS_SEEK_SET = 0,
    /** the current position */
    VFS_SEEK_CUR = 1,
    /** the end of the file */
    VFS_SEEK_END = 2,
} e_VFSSEEKMODES;

/**
 * VFS PUBLIC API END
 */

#endif

#ifdef _VFS_H_INTERNAL

typedef struct {
    const char *mountpoint;
    const t_VFSOperations *ops;
    const t_FSContext *ctx;
} t_MountPoint;

/**
 * a vnode, the vfs's cached copy of a file. they form a tree
 *  and each remembers which driver it belongs to
 */
typedef struct t_VFSNode {
    struct t_VFSNode *parent,
    /** linked list of children */
                     *children,
                     *next;
    /** the driver's handle for the file */
    t_FSNode *handle;
    /** the driver that owns the file */
    t_VFSOperations *driver;
} t_VFSNode;

/**
 * a file descriptor object which
 * 	serves as the final layer of
 * 	abstraction for files in the
 * 	operating systems - this will
 * 	be the final descriptor returned
 * 	in userspace when users want
 * 	to interact with a file
 */
typedef struct t_FileDescriptor {
    /** the open file, from the driver */
    t_FSFile descriptor;
    /** the driver the open file belongs to */
    t_VFSOperations *driver
} t_FileDescriptor;

/**
 * @brief creates a file using the VFS -> FS API,
 * 	uses the filesystem of the parent file
 * @param parent the directory to create it in
 * @param name the file name
 * @param attribs e_FileAttribs or'd together
 * @return 0
 */
size_t VFS_make_file
    (t_VFSNode *parent, const char *name, uint8_t attribs);

/**
 * @brief returns a vnode to a file with given name that
 * 	is contained in the parent vnode directory
 *  checks the children already cached first, then asks the driver
 * @param parent the directory vnode
 * @param name the file name
 * @return the vnode, NULL if there is no such file
 */
t_VFSNode *VFS_lookup(t_VFSNode *parent, const char *name);

/**
 * @brief returns a vnode to a file represented
 * 	by the given string path
 * @param _path full path from the root
 * @return the vnode, NULL if any part of the path doesn't exist
 */
t_VFSNode *VFS_walk_path(const char *_path);

/**
 * @brief makes and allocates a new vnode given the parent vnode
 * 	and the underlying handle
 *  it uses the parent's driver and isn't linked into the parent yet
 * @param parent the parent vnode
 * @param handle the driver's handle for the file
 * @return the new vnode
 */
t_VFSNode *VFS_make_vnode(t_VFSNode *parent, t_FSNode *handle);

/**
 * @brief inserts a vnode into a directory vnode
 *  (at the end of its children)
 * @param parent the directory vnode
 * @param child the vnode to add
 */
void VFS_insert_vnode(t_VFSNode *parent, t_VFSNode *child);

/**
 * @brief returns the name of the file in *out_fname and the
 * 	vnode for the directory in the actual return
 * 	value
 * @param _path full path of the file
 * @param out_fname set to the file name, which points into _path
 * @return the vnode of the directory it's in
 */
t_VFSNode *VFS_get_dir_and_fname(const char *_path, char **out_fname);


#endif
