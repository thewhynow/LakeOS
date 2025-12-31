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

/**
 * Initializes the virtual file manager
 *  and allocates memory for all the
 *  relevant files and stuff
 */
void VFM_init();
/**
 * Creates a virtual file and returns a
 *  handle to it
 */
t_VFMHandle *VFM_create(const char *name, uint8_t atrribs);
void VFM_remove(t_VFMHandle *file);
size_t VFM_read(t_VFMFile *file, size_t bytes, void *buff);
size_t VFM_write(t_VFMFile *file, size_t bytes, const void *buff);
/**
 * instead of appending data to the virtual
 *  file buffer, this function pushes data
 *  to the back of the buffer, acting as a
 *  queue pusher
 */
size_t VFM_pushqueue(t_VFMFile *file, size_t bytes, const void *buff);
const char *VFM_nodename(t_VFMHandle *file);
void VFM_fstat(t_VFMHandle *file, t_FileStat *out);

#endif
