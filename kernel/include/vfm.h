#include "sal.h"
#include "vfs.h"

#ifdef _VFM_H_INTERNAL

typedef struct t_VFMFile {
    t_FileChrono modified;
    struct t_VFMFile *next;
    char *content,
         *name;
    size_t pos;
    uint8_t opflags,  // 0 == closed 
            fileflags;
} t_VFMFile, 
/* no reason to abstract handle beyond file */
  t_VFMHandle;

typedef struct {
    t_FileChrono start_chrono;
    t_VFMHandle *virt_files;
} t_VFMContext;

void VFM_free_file(t_VFMFile *file);

t_VFMFile *VFM_new_file();

#else

typedef void t_VFMHandle,
             t_VFMContext,
             t_VFMFile;
#endif

#ifndef _VFM_H
#define _VFM_H

extern t_VFSOperations vfm_vfs_ops;


/**
 * for vfs api
 */


/**
 * Initializes the virtual file manager
 *  and allocates memory for all the
 *  relevant files and stuff,
 *  returns nothing really
 */
t_FSContext VFM_init();

/**
 * Uninitializes the virtual file manager
 * 	and frees memory for all the relevant
 * 	files and stuff
 */
void VFM_uninit();

/**
 * returns the root handle of the
 * 	virtual file directory
 */
t_FSNode VFM_root();

/**
 * returns a t_FSNode that points
 * 	to the virtual file <name>
 */
t_VFMHandle *VFM_lookup(t_FSNode parent, const char *name);

/**
 * Creates a virtual file and returns a
 *  handle to it
 */
t_VFMHandle *VFM_create(t_VFMFile *parent, const char *name, uint8_t atrribs);

/**
 * Revmoes a virtual file and frees all
 * 	its corresponding memory
 */
void VFM_remove(t_VFMHandle *file);

/**
 * Does nothing for now.. might retrieve the
 * 	cached data from disk later
 */
t_VFMFile *VFM_open(t_VFMFile *file, uint8_t mode);

/**
 * Again does nothing for now.. might
 * 	cache the file contents to disk later
 */
void VFM_close(t_VFMFile *file);

/**
 * Read n bytes from a virtual file
 */
size_t VFM_read(t_VFMFile *file, size_t bytes, void *buff);

/**
 * Write n bytes to a virtual file
 */
size_t VFM_write(t_VFMFile *file, size_t bytes, const void *buff);

/**
 * instead of appending data to the virtual
 *  file buffer, this function pushes data
 *  to the back of the buffer, acting as a
 *  queue pusher
 */
size_t VFM_writeback(t_VFMFile *file, size_t bytes, const void *buff);

t_VFMFile *VFM_ReadDir(t_VFMFile *dir, size_t n);

/**
 * gets the name of a file given the handle
 */
const char *VFM_nodename(t_VFMHandle *file);

/**
 * impelments fstat for virtual files
 */
void VFM_fstat(t_VFMHandle *file, t_FileStat *out);

#endif
